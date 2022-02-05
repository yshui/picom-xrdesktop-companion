#![feature(
    never_type,
    backtrace,
    map_try_insert,
    box_into_inner,
    downcast_unchecked
)]
use std::sync::Arc;

use anyhow::{anyhow, Context};
use futures::{StreamExt, TryStreamExt};
use gio::prelude::*;
use glib::translate::ToGlibPtr;
use log::*;
use tokio::{
    sync::Mutex,
    task::{block_in_place, spawn_blocking},
};
use x11rb::{
    connection::Connection,
    protocol::{
        composite::ConnectionExt as _,
        damage::{self, ConnectionExt as _},
        xproto::{self, ConnectionExt as _},
    },
    rust_connection::RustConnection,
};
use xrd::{ClientExt, ClientExtExt, DesktopCursorExt, WindowExt};

mod gl;
mod picom;
mod utils;

const PIXELS_PER_METER: f32 = 600.0;
type Result<T> = anyhow::Result<T>;

#[allow(non_camel_case_types, dead_code)]
mod inputsynth {
    include!(concat!(env!("OUT_DIR"), "/inputsynth.rs"));
}

struct InputSynth(Option<&'static mut inputsynth::InputSynth>);
unsafe impl Send for InputSynth {}
unsafe impl Sync for InputSynth {}
impl InputSynth {
    fn new() -> Option<Self> {
        unsafe {
            inputsynth::input_synth_new(inputsynth::InputsynthBackend::INPUTSYNTH_BACKEND_XDO)
                .as_mut()
        }
        .map(Some)
        .map(Self)
    }
    fn move_cursor(&mut self, x: i32, y: i32) {
        unsafe {
            inputsynth::input_synth_move_cursor(
                self.0.as_mut().map(|ptr| (*ptr) as *mut _).unwrap(),
                x,
                y,
            );
        }
    }
    fn click(&mut self, x: i32, y: i32, button: i32, pressed: bool) {
        unsafe {
            inputsynth::input_synth_click(
                self.0.as_mut().map(|ptr| (*ptr) as *mut _).unwrap(),
                x,
                y,
                button,
                pressed as _,
            )
        }
    }
    fn character(&mut self, key: i8) {
        unsafe {
            inputsynth::input_synth_character(
                self.0.as_mut().map(|ptr| (*ptr) as *mut _).unwrap(),
                key,
            )
        }
    }
}
impl Drop for InputSynth {
    fn drop(&mut self) {
        if let Some(ptr) = self.0.as_mut().map(|x| (*x) as *mut inputsynth::InputSynth) {
            unsafe {
                gobject_sys::g_object_unref(ptr as *mut _);
            }
        }
        self.0 = None;
    }
}

#[derive(Debug)]
struct TextureSet {
    x11_pixmap: xproto::Pixmap,
    x11_texture: gl::Texture,
    remote_texture: gulkan::Texture,
    imported_texture: gl::Texture,
}

impl TextureSet {
    fn free(this: Option<Self>, gl: &gl::Gl, x11: &RustConnection) -> Result<()> {
        if let Some(Self {
            x11_texture,
            x11_pixmap,
            ..
        }) = this
        {
            gl.release_texture_sync(x11_texture)?;
            x11.free_pixmap(x11_pixmap)?.check()?;
        }
        Ok(())
    }
}

#[derive(Debug)]
struct Window {
    id: xproto::Window,
    gl: gl::Gl,
    damage: damage::Damage,
    x11: Arc<RustConnection>,
    xrd: Arc<Mutex<xrd::Client>>,
    textures: Option<TextureSet>,
    xrd_window: xrd::Window,
}

impl Drop for Window {
    fn drop(&mut self) {
        let textures = self.textures.take();
        let gl = self.gl.clone();
        let x11 = self.x11.clone();
        // damage will have already been freed is window is closed
        // so don't check for error
        let xrd = self.xrd.blocking_lock();
        xrd.remove_window(&self.xrd_window);
        self.xrd_window.close();
        x11.damage_destroy(self.damage).unwrap();
        TextureSet::free(textures, &gl, &x11).unwrap();
    }
}

#[derive(Debug)]
struct Cursor {
    texture: gulkan::Texture,
    hotspot_x: u32,
    hotspot_y: u32,
}

type WindowMap = Mutex<std::collections::HashMap<u32, Window>>;

struct App {
    gl: gl::Gl,
    dbus: zbus::Connection,
    windows: WindowMap,
    xrd_client: Arc<Mutex<xrd::Client>>,
    input_synth: Mutex<InputSynth>,
    x11: Arc<RustConnection>,
    screen: u32,
    display: String,
    cursors: Mutex<std::collections::HashMap<u32, Cursor>>,
}

#[derive(Debug)]
enum InputEvent {
    Move {
        wid: u32,
        x: f32,
        y: f32,
    },
    Click {
        wid: u32,
        x: f32,
        y: f32,
        button: xrd::sys::XrdInputSynthButton,
        pressed: bool,
    },
    KeyPresses {
        string: Vec<i8>,
    },
}

impl std::fmt::Debug for App {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "App")
    }
}
impl App {
    async fn new() -> Result<Self> {
        if !xrd::settings_is_schema_installed() {
            return Err(anyhow!("xrdesktop GSettings Schema not installed"));
        }

        let dbus = zbus::Connection::session().await.unwrap();

        let settings = xrd::settings_get_instance().unwrap();
        let mode: xrd::ClientMode =
            unsafe { glib::translate::from_glib(settings.enum_("default-mode")) };
        println!("{}", mode);

        if mode == xrd::ClientMode::Scene {
            unimplemented!("Scene mode");
        }

        let client = xrd::Client::with_mode(mode);
        let input_synth = Mutex::new(InputSynth::new().expect("Failed to initialize inputsynth"));
        let (x11, screen) = RustConnection::connect(None)?;
        let x11 = Arc::new(x11);
        block_in_place(|| {
            use x11rb::protocol::xfixes::{ConnectionExt, CursorNotifyMask};
            let (damage_major, damage_minor) = x11rb::protocol::damage::X11_XML_VERSION;
            x11.damage_query_version(damage_major, damage_minor)?
                .reply()?;
            let (xfixes_major, xfixes_minor) = x11rb::protocol::xfixes::X11_XML_VERSION;
            x11.xfixes_query_version(xfixes_major, xfixes_minor)?
                .reply()?;
            x11.xfixes_select_cursor_input(
                x11.setup().roots[screen].root,
                CursorNotifyMask::DISPLAY_CURSOR,
            )?
            .check()?;
            Result::Ok(())
        })?;

        Ok(Self {
            gl: gl::Gl::new(x11.clone(), screen as u32).await?,
            dbus,
            windows: Default::default(),
            xrd_client: Arc::new(Mutex::new(client)),
            input_synth,
            screen: screen as u32,
            x11,
            display: std::env::var("DISPLAY").unwrap().replace(':', "_"),
            cursors: Default::default(),
        })
    }

    // Change cursor to the one identified by cursor_serial, if it was cached; otherwise, fetch the
    // current cursor (might or might not be cursor_serial) and set the cursor to that.
    async fn refresh_cursor(&self, cursor_serial: u32) -> Result<()> {
        use x11rb::protocol::xfixes;
        let xrd_client = self.xrd_client.lock().await;
        let mut cursors = self.cursors.lock().await;
        let cursor = if let Some(cursor) = cursors.get(&cursor_serial) {
            cursor
        } else {
            let cursor_image = block_in_place(|| {
                use xfixes::ConnectionExt;
                Result::Ok(self.x11.xfixes_get_cursor_image()?.reply()?)
            })?;
            let image_data = unsafe {
                std::slice::from_raw_parts(
                    cursor_image.cursor_image.as_ptr() as *const _,
                    cursor_image.cursor_image.len() * std::mem::size_of::<u32>(),
                )
            };
            use gdk_pixbuf::Colorspace;
            let pixbuf = gdk_pixbuf::Pixbuf::from_bytes(
                &glib::Bytes::from(image_data),
                Colorspace::Rgb,
                true,
                8,
                cursor_image.width.into(),
                cursor_image.height.into(),
                4i32 * cursor_image.width as i32,
            );
            let texture = {
                let gulkan_client = xrd_client.gulkan().unwrap();
                let layout = xrd_client.upload_layout();
                let texture: gulkan::Texture = unsafe {
                    glib::translate::from_glib_full(gulkan::sys::gulkan_texture_new_from_pixbuf(
                        gulkan_client.as_ptr(),
                        pixbuf.as_ptr(),
                        ash::vk::Format::R8G8B8A8_SRGB.as_raw() as _,
                        layout,
                        false as _,
                    ))
                };
                texture
            };
            cursors.entry(cursor_image.cursor_serial).or_insert(Cursor {
                hotspot_x: cursor_image.xhot.into(),
                hotspot_y: cursor_image.yhot.into(),
                texture,
            })
        };
        let xrd_cursor = xrd_client.desktop_cursor().unwrap();
        xrd_cursor.set_and_submit_texture(&cursor.texture);
        xrd_cursor.set_hotspot(cursor.hotspot_x as _, cursor.hotspot_y as _);
        Ok(())
    }

    async fn handle_x_events(&self, event: x11rb::protocol::Event) -> Result<()> {
        use x11rb::protocol::xfixes;
        use x11rb::protocol::Event;
        match event {
            Event::DamageNotify(damage::NotifyEvent { drawable, .. }) => {
                let mut windows = self.windows.lock().await;
                let w = windows.get_mut(&drawable).unwrap();
                block_in_place(|| {
                    Result::Ok(
                        self.x11
                            .damage_subtract(w.damage, x11rb::NONE, x11rb::NONE)?
                            .check()?,
                    )
                })?;
                self.render_win(w).await?;
            }
            Event::XfixesCursorNotify(xfixes::CursorNotifyEvent { cursor_serial, .. }) => {
                self.refresh_cursor(cursor_serial).await?;
            }
            _ => (),
        }
        Ok(())
    }

    async fn handle_input_events(&self, input_event: InputEvent) {
        trace!("{:?}", input_event);
        let raise_window_and_resolve_position = |wid, x, y| {
            let geometry = block_in_place(|| {
                let cookie1 = self
                    .x11
                    .configure_window(
                        wid,
                        &xproto::ConfigureWindowAux {
                            stack_mode: Some(xproto::StackMode::ABOVE),
                            ..Default::default()
                        },
                    )
                    .unwrap();
                let cookie2 = self.x11.get_geometry(wid).unwrap();
                cookie1.check()?;
                Result::Ok(cookie2.reply()?)
            })?;
            let x = (geometry.x as f32 + x) as _;
            let y = (geometry.y as f32 + y) as _;
            Result::Ok((x, y))
        };

        match input_event {
            InputEvent::Move { x, y, wid } => {
                if let Ok((x, y)) = raise_window_and_resolve_position(wid, x, y) {
                    // The window could have been closed, in that case we stop
                    self.input_synth.lock().await.move_cursor(x, y);
                }
            }

            InputEvent::Click {
                x,
                y,
                wid,
                button,
                pressed,
            } => {
                if let Ok((x, y)) = raise_window_and_resolve_position(wid, x, y) {
                    // The window could have been closed, in that case we stop
                    self.input_synth.lock().await.click(x, y, button, pressed);
                }
            }
            InputEvent::KeyPresses { string } => {
                let mut input_synth = self.input_synth.lock().await;
                block_in_place(|| {
                    for ch in string {
                        input_synth.character(ch);
                    }
                });
            }
        }
    }

    async fn run(self: Self) -> Result<()> {
        let this = Arc::new(self);
        Self::setup_initial_windows(&this).await?;
        this.refresh_cursor(0).await?;
        this.xrd_client
            .lock()
            .await
            .desktop_cursor()
            .unwrap()
            .show();

        let picom = picom::CompositorProxy::builder(&this.dbus)
            .destination(format!("com.github.chjj.compton.{}", this.display))?
            .build()
            .await?;

        let (tx, mut x11_rx) = tokio::sync::mpsc::channel(4);
        let x11_clone = this.x11.clone();

        // tokio task for receiving X events
        let _: tokio::task::JoinHandle<Result<!>> = spawn_blocking(move || loop {
            let event = x11_clone.wait_for_event()?;
            tx.blocking_send(event)?;
            while let Some(event) = x11_clone.poll_for_event()? {
                tx.blocking_send(event)?;
            }
        });
        let (mut exit_rx, mut input_rx) = {
            let xrd_client = this.xrd_client.lock().await;
            let (input_tx, input_rx) = tokio::sync::mpsc::channel(2);
            let tx = input_tx.clone();
            xrd_client.connect_move_cursor_event(move |_, event| {
                if event.ignore != 0 {
                    return;
                }
                let window: xrd::Window = unsafe { glib::translate::from_glib_none(event.window) };
                let point: graphene::Point =
                    unsafe { glib::translate::from_glib_none(event.position) };
                let mut native = 0u64;
                unsafe {
                    gobject_sys::g_object_get(
                        window.as_ptr() as *mut _,
                        "native\0".as_bytes().as_ptr() as *const _,
                        &mut native,
                        0,
                    );
                };
                // If the queue is full, we drop the event
                let _: std::result::Result<_, _> = tx.try_send(InputEvent::Move {
                    wid: native as u32,
                    x: point.x(),
                    y: point.y(),
                });
            });
            let tx = input_tx.clone();
            xrd_client.connect_click_event(move |_, event| {
                let window: xrd::Window = unsafe { glib::translate::from_glib_none(event.window) };
                let point: graphene::Point =
                    unsafe { glib::translate::from_glib_none(event.position) };
                let mut native = 0u64;
                unsafe {
                    gobject_sys::g_object_get(
                        window.as_ptr() as *mut _,
                        "native\0".as_bytes().as_ptr() as *const _,
                        &mut native,
                        0,
                    );
                };
                // We don't want to lose click events
                tx.blocking_send(InputEvent::Click {
                    wid: native as u32,
                    x: point.x(),
                    y: point.y(),
                    button: event.button,
                    pressed: event.state != 0,
                })
                .unwrap();
            });
            let tx = input_tx.clone();
            xrd_client.connect_keyboard_press_event(move |_, event| {
                let event: &gdk::EventKey = event.downcast_ref().unwrap();
                let string = unsafe {
                    std::slice::from_raw_parts(event.as_ref().string, event.length() as _)
                };
                let string = string.to_owned();
                tx.blocking_send(InputEvent::KeyPresses { string }).unwrap();
            });

            let (tx, exit_rx) = tokio::sync::mpsc::channel(1);
            xrd_client.connect_request_quit_event(move |_, _| {
                tx.blocking_send(()).unwrap();
            });

            (exit_rx, input_rx)
        };

        let mut win_mapped = picom.receive_win_mapped().await?;
        let mut win_unmapped = picom.receive_win_unmapped().await?;

        info!("Existing windows mapped, entering mainloop");
        loop {
            tokio::select! {
                event = x11_rx.recv() => {
                    trace!("{:?}", event);
                    this.handle_x_events(event.with_context(|| anyhow!("Xorg connection broke"))?).await?;
                }
                new_window = win_mapped.next() => {
                    let new_window = new_window.with_context(|| anyhow!("dbus connection broke"))?;
                    this.map_win(new_window.args()?.wid).await?;
                }
                closed_window = win_unmapped.next() => {
                    let closed_window = closed_window.with_context(|| anyhow!("dbus connection broke"))?;
                    let w = this.windows.lock().await.remove(&closed_window.args()?.wid);
                    block_in_place(|| drop(w));
                }
                input_event = input_rx.recv() => {
                    let input_event = input_event.unwrap();
                    this.handle_input_events(input_event).await;
                }
                _ = exit_rx.recv() => {
                    break;
                }
            }
        }
        // Ensure `App` in dropped in a sync context
        let this = Arc::try_unwrap(this).unwrap();
        block_in_place(|| drop(this));
        Ok(())
    }

    async fn refresh_texture(&self, w: &mut Window) -> Result<bool> {
        let x11_clone = self.x11.clone();
        let wid = w.id;
        let win_geometry =
            block_in_place(|| Result::Ok(x11_clone.as_ref().get_geometry(wid)?.reply()?))?;
        if let Some((width, height)) = w
            .textures
            .as_ref()
            .map(|ts| (ts.x11_texture.width(), ts.x11_texture.height()))
        {
            if width != win_geometry.width.into() || height != win_geometry.height.into() {
                info!("Free old textures for {}", wid);
                block_in_place(|| TextureSet::free(w.textures.take(), &self.gl, &self.x11))?;
            }
        }

        if w.textures.is_none() {
            let (attrs, x11_pixmap) = block_in_place(|| {
                let attrs = self.x11.get_window_attributes(wid)?.reply()?;
                let x11_pixmap = self.x11.generate_id()?;
                self.x11
                    .composite_name_window_pixmap(wid, x11_pixmap)?
                    .check()?;
                Result::Ok((attrs, x11_pixmap))
            })?;
            let x11_texture = self.gl.bind_texture(x11_pixmap, attrs.visual).await?;

            let (remote_texture, fd, size) = {
                let xrd_client = self.xrd_client.lock().await; // Need to keep this alive for gulkan_client
                let gulkan_client = xrd_client.gulkan().unwrap();
                let extent = ash::vk::Extent2D {
                    width: win_geometry.width.into(),
                    height: win_geometry.height.into(),
                };
                let layout = xrd_client.upload_layout();

                let mut size = 0;
                let mut fd = 0;
                let remote_texture = unsafe {
                    glib::translate::from_glib_full(gulkan::sys::gulkan_texture_new_export_fd(
                        gulkan_client.as_ptr(),
                        std::mem::transmute(extent),
                        ash::vk::Format::R8G8B8A8_SRGB.as_raw() as _,
                        layout,
                        &mut size,
                        &mut fd,
                    ))
                };
                (remote_texture, fd, size)
            };
            let imported_texture = self
                .gl
                .import_fd(
                    win_geometry.width.into(),
                    win_geometry.height.into(),
                    fd,
                    size as _,
                )
                .await?;
            w.textures = Some(TextureSet {
                x11_texture,
                x11_pixmap,
                remote_texture,
                imported_texture,
            });
            Ok(true)
        } else {
            Ok(false)
        }
    }

    async fn render_win(&self, w: &mut Window) -> Result<()> {
        if !w.xrd_window.is_visible() {
            //return Ok(());
        }

        #[cfg(debug_assertions)]
        self.gl.capture(true).await?;

        let refreshed = self.refresh_texture(w).await?;
        let textures = w.textures.as_ref().unwrap();
        self.gl
            .blit(&textures.x11_texture, &textures.imported_texture)
            .await?;

        #[cfg(debug_assertions)]
        self.gl.capture(false).await?;

        if refreshed {
            w.xrd_window
                .set_and_submit_texture(&textures.remote_texture);
        } else {
            w.xrd_window.submit_texture();
        }
        Ok(())
    }

    async fn map_win(&self, wid: u32) -> Result<()> {
        let picom_service = format!("com.github.chjj.compton.{}", self.display);
        let proxy = picom::WindowProxy::builder(&self.dbus)
            .destination(picom_service)?
            .path(format!("{}/{}/{}", PICOM_OBJECT_PATH, "windows", wid))
            .map(|pb| pb.cache_properties(zbus::CacheProperties::No))?
            .build()
            .await?;
        if !proxy.mapped().await? {
            return Ok(());
        }
        let ty = proxy.type_().await?;
        if ty != "normal" && ty != "dock" {
            return Ok(());
        }
        let window_name = proxy.name().await?;
        // TODO: cache root geometry
        let root_win = self.x11.setup().roots[self.screen as usize].root;
        let (root_geometry, win_geometry) = block_in_place(|| {
            Result::Ok((
                self.x11.get_geometry(root_win)?.reply()?,
                self.x11.get_geometry(wid)?.reply()?,
            ))
        })?;
        if win_geometry.x <= -(win_geometry.width as i16)
            || win_geometry.y <= -(win_geometry.height as i16)
            || win_geometry.x >= root_geometry.width as _
            || win_geometry.y >= root_geometry.height as _
        {
            // If the window is entirely outside of the screen, hide it
            // Firefox does this and has a 1x1 window outside the screen
            return Ok(());
        }
        println!("{}", wid);

        let xrd_window = {
            let xrd_client = self.xrd_client.lock().await;
            let xrd_window = xrd::Window::new_from_pixels(
                &*xrd_client,
                &window_name,
                win_geometry.width.into(),
                win_geometry.height.into(),
                PIXELS_PER_METER,
            )
            .with_context(|| anyhow::anyhow!("failed to create xrdWindow"))?;
            unsafe {
                let pspec = glib::ObjectExt::find_property(&xrd_window, "native").unwrap();
                gobject_sys::g_object_set(
                    xrd_window.as_object_ref().to_glib_none().0,
                    pspec.name().as_ptr() as *const _,
                    wid as *const std::ffi::c_void,
                    0,
                );
                xrd::sys::xrd_client_add_window(
                    xrd_client.as_ptr(),
                    xrd_window.as_ptr(),
                    true as _,
                    std::mem::transmute(wid as usize),
                )
            };
            xrd_window
        };

        let point = graphene::Point3D::new(
            (win_geometry.x + win_geometry.width as i16 / 2 - root_geometry.width as i16 / 2)
                as f32
                / PIXELS_PER_METER,
            (win_geometry.y + win_geometry.height as i16 / 2 - root_geometry.height as i16 * 3 / 4)
                as f32
                / PIXELS_PER_METER,
            self.windows.lock().await.len() as f32 / 3.0 - 8.0,
        );
        let mut transform = graphene::Matrix::new_translate(&point);
        xrd_window.set_transformation(&mut transform);
        xrd_window.set_reset_transformation(&mut transform);

        let damage = self.x11.generate_id()?;
        let x11_clone = self.x11.clone();
        {
            let mut windows = self.windows.lock().await;
            block_in_place(move || {
                x11_clone
                    .damage_create(damage, wid, x11rb::protocol::damage::ReportLevel::NON_EMPTY)?
                    .check()?;
                Result::Ok(())
            })?;

            let window = Window {
                id: wid,
                gl: self.gl.clone(),
                x11: self.x11.clone(),
                xrd_window,
                xrd: self.xrd_client.clone(),
                damage,
                textures: None,
            };
            let window = windows.try_insert(wid, window).unwrap();
            self.render_win(window).await?;
        }
        Ok(())
    }

    async fn setup_initial_windows(self: &Arc<Self>) -> Result<()> {
        let picom_service = format!("com.github.chjj.compton.{}", self.display);
        let proxy: zbus::Proxy<'_> = zbus::ProxyBuilder::new_bare(&self.dbus)
            .destination(picom_service)?
            .interface("what.ever")?
            .path(format!("{}/{}", PICOM_OBJECT_PATH, "windows"))?
            .build()
            .await?;

        let windows = zbus::xml::Node::from_reader(proxy.introspect().await?.as_bytes())?;
        let futs: futures::stream::FuturesUnordered<_> = windows
            .nodes()
            .into_iter()
            .map(|w| {
                let self_clone = self.clone();
                let wid = w.name().unwrap().to_owned();
                tokio::spawn(async move {
                    if let Ok(wid) = parse_int::parse(&wid) {
                        if let Err(e) = self_clone.map_win(wid).await {
                            error!("Failed to map window {}, {}", wid, e);
                        }
                    } else {
                        error!("Invalid window id from picom: {}", wid);
                    }
                })
            })
            .collect();
        let () = futs.try_collect().await?;
        Ok(())
    }
}

const PICOM_OBJECT_PATH: &str = "/com/github/chjj/compton";
type RenderDoc = renderdoc::RenderDoc<renderdoc::V141>;
fn maybe_load_renderdoc() -> Option<RenderDoc> {
    use libloading::os::unix::{Library, RTLD_NOW};
    let lib = unsafe { Library::open(Some("librenderdoc.so"), libc::RTLD_NOLOAD | RTLD_NOW) };
    if lib.is_ok() {
        info!("RenderDoc loaded");
        Some(RenderDoc::new().unwrap())
    } else {
        info!("RenderDoc not loaded {:?}", lib);
        None
    }
}
lazy_static::lazy_static! {
    pub static ref RENDERDOC: std::sync::Mutex<Option<RenderDoc>> =
        std::sync::Mutex::new(maybe_load_renderdoc());
}
#[tokio::main]
async fn main() -> Result<()> {
    env_logger::builder().format_timestamp_millis().init();
    std::thread::spawn(|| {
        let l = glib::MainLoop::new(None, false);
        l.run();
    });
    let ctx = App::new().await?;
    ctx.run().await?;
    info!("App exited");
    Ok(())
}
