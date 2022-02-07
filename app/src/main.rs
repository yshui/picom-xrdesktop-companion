#![feature(
    never_type,
    backtrace,
    map_try_insert,
    box_into_inner,
    downcast_unchecked
)]
use std::{
    collections::{hash_map::OccupiedError, HashMap},
    sync::Arc,
};

use anyhow::{anyhow, Context};
use drop_bomb::DropBomb;
use futures::{StreamExt, TryStreamExt};
use gio::prelude::*;
use glib::translate::ToGlibPtr;
use log::*;
use tokio::{
    sync::{Mutex, RwLock},
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

x11rb::atom_manager! {
    pub AtomCollection: AtomCollectionCookie {
        WM_TRANSIENT_FOR,
    }
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
    async fn free(this: Option<Self>, gl: &gl::Gl, x11: &RustConnection) -> Result<()> {
        if let Some(Self {
            x11_texture,
            x11_pixmap,
            ..
        }) = this
        {
            x11.free_pixmap(x11_pixmap)?.check()?;
            gl.release_texture(x11_texture).await?;
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
    xrd_window: Mutex<xrd::Window>,
    client_wid: u32,

    // Dropping Window is unsafe, so we don't allow implicit dropping
    drop_bomb: DropBomb,
}

impl Window {
    // Unsafe, must be called with the exclusive access to WindowState
    unsafe fn unlink_window(win: &mut xrd::Window) {
        use glib::translate::from_glib_none;
        // Remove from window group
        let data = xrd::sys::xrd_window_get_data(win.as_ptr());
        if (*data).parent_window.is_null() || (*data).child_window.is_null() {
            return;
        }
        // If either parent or child is null, xrd_window_close handles it fine
        // Otherwise we connect our child to our parent.
        let offset: graphene::Point = from_glib_none(&(*data).child_offset_center as *const _);
        let child_offset: graphene::Point =
            from_glib_none(&(*(*data).child_window).child_offset_center as *const _);
        let mut offset =
            graphene::Point::new(offset.x() + child_offset.x(), offset.y() + child_offset.y());
        let parent: xrd::Window = from_glib_none((*(*data).parent_window).xrd_window);
        let child: xrd::Window = from_glib_none((*(*data).child_window).xrd_window);
        parent.add_child(&child, &mut offset);
    }
    // Must either be dropped with exclusive access to WindowState
    unsafe fn drop(self) -> impl std::future::Future<Output = Result<()>> {
        let Self {
            xrd,
            damage,
            mut drop_bomb,
            x11,
            gl,
            xrd_window,
            textures,
            ..
        } = self;
        drop_bomb.defuse();

        let mut xrd_window = xrd_window.into_inner();
        Self::unlink_window(&mut xrd_window);
        // We do this so the future doesn't capture Window, which can lead to <Window as
        // Drop>::drop being called
        async move {
            let xrd = xrd.lock().await;
            xrd.remove_window(&xrd_window);
            xrd_window.close();
            // damage will have already been freed is window is closed
            // so ignore error
            x11.damage_destroy(damage).unwrap().ignore_error();
            TextureSet::free(textures, &gl, &x11).await
        }
    }
}

#[derive(Debug)]
struct Cursor {
    texture: gulkan::Texture,
    hotspot_x: u32,
    hotspot_y: u32,
}

#[derive(Default, Debug)]
struct WindowState {
    windows: HashMap<u32, RwLock<Window>>,
    client_window_to_window: HashMap<u32, u32>,
}

struct App {
    gl: gl::Gl,
    dbus: zbus::Connection,
    xrd_client: Arc<Mutex<xrd::Client>>,
    input_synth: Mutex<InputSynth>,
    x11: Arc<RustConnection>,
    screen: u32,
    display: String,
    cursors: Mutex<std::collections::HashMap<u32, Cursor>>,
    atoms: AtomCollection,
    window_state: RwLock<WindowState>,
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
        let atoms = AtomCollection::new(&*x11)?.reply()?;

        Ok(Self {
            gl: gl::Gl::new(x11.clone(), screen as u32).await?,
            dbus,
            window_state: Default::default(),
            xrd_client: Arc::new(Mutex::new(client)),
            input_synth,
            screen: screen as u32,
            x11,
            display: std::env::var("DISPLAY").unwrap().replace(':', "_"),
            cursors: Default::default(),
            atoms,
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
            debug!("new cursor {}", cursor_image.cursor_serial);
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
                let window_state = self.window_state.read().await;
                if let Some(w) = window_state.windows.get(&drawable) {
                    // we might not be able to find the window if:
                    // we receive a damage notify from X, but have processed it;
                    // then we receive a WinUnmapped signal from picom, and we processed it;
                    // then we dequeue the damage notify from x11rb.
                    // this is not an error.
                    let mut w = w.write().await;

                    // Window could've closed between damage_notify and here, handle that case.
                    if block_in_place(|| {
                        Result::Ok(
                            self.x11
                                .damage_subtract(w.damage, x11rb::NONE, x11rb::NONE)?
                                .check()?,
                        )
                    })
                    .is_ok()
                    {
                        // render_win will fail if window is closed, this is fine.
                        let _: Result<_> = self.render_win(&mut w).await;
                    }
                }
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
                debug!("key press {:?}", string);
                let mut input_synth = self.input_synth.lock().await;
                block_in_place(|| {
                    for ch in string {
                        if ch == b'\n' as _ {
                            input_synth.character(b'\r' as _);
                        } else {
                            input_synth.character(ch);
                        }
                    }
                });
            }
        }
    }

    // A window group in xrdesktop is a linked list held together by the window's parent/child
    // pointers. this function finds the group for `wid`, and returns the last window in the list
    async fn find_window_group(window_state: &WindowState, wid: u32) -> Option<&RwLock<Window>> {
        debug!("looking for group for {}", wid);
        let window = window_state.windows.get(&wid).or_else(|| {
            window_state
                .client_window_to_window
                .get(&wid)
                .and_then(|p| {
                    debug!("using parent {} of client window {} instead", p, wid);
                    window_state.windows.get(p)
                })
        })?;
        let parent = unsafe {
            let mut current =
                xrd::sys::xrd_window_get_data(window.read().await.xrd_window.lock().await.as_ptr());
            while !(*current).child_window.is_null() {
                debug!("{}", (*current).native as u64);
                current = (*current).child_window;
            }
            current
        };

        let native: u64 = unsafe { (*parent).native } as _;
        Some(window_state.windows.get(&(native as _)).unwrap())
    }

    pub async fn run(self: Arc<Self>) -> Result<()> {
        Self::setup_initial_windows(&self).await?;
        self.refresh_cursor(0).await?;
        self.xrd_client
            .lock()
            .await
            .desktop_cursor()
            .unwrap()
            .show();

        let picom = picom::CompositorProxy::builder(&self.dbus)
            .destination(format!("com.github.chjj.compton.{}", self.display))?
            .build()
            .await?;

        let (tx, mut x11_rx) = tokio::sync::mpsc::channel(4);
        let x11_clone = self.x11.clone();

        // tokio task for receiving X events
        let _: tokio::task::JoinHandle<Result<!>> = spawn_blocking(move || loop {
            let event = x11_clone.wait_for_event()?;
            tx.blocking_send(event)?;
            while let Some(event) = x11_clone.poll_for_event()? {
                tx.blocking_send(event)?;
            }
        });
        let (mut exit_rx, mut input_rx) = {
            let xrd_client = self.xrd_client.lock().await;
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
            // if send() errors, that means run() has returned. so ignore those errors
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
                        &mut native as *mut _,
                        0,
                    );
                };
                // We don't want to lose click events
                let _ = tx.blocking_send(InputEvent::Click {
                    wid: native as u32,
                    x: point.x(),
                    y: point.y(),
                    button: event.button,
                    pressed: event.state != 0,
                });
            });
            let tx = input_tx;
            xrd_client.connect_keyboard_press_event(move |_, event| {
                let event: &gdk::EventKey = event.downcast_ref().unwrap();
                let string = unsafe {
                    std::slice::from_raw_parts(event.as_ref().string, event.length() as _)
                };
                let string = string.to_owned();
                let _ = tx.blocking_send(InputEvent::KeyPresses { string });
            });

            let (tx, exit_rx) = tokio::sync::mpsc::channel(1);
            xrd_client.connect_request_quit_event(move |_, _| {
                let _ = tx.blocking_send(());
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
                    let this = self.clone();
                    let event = event.with_context(|| anyhow!("Xorg connection broke"))?;
                    tokio::spawn(async move {
                        if let Err(e) = this.handle_x_events(event).await {
                            error!("Failed to handle X events {}", e);
                        }
                    });
                }
                new_window = win_mapped.next() => {
                    let new_window = new_window.with_context(|| anyhow!("dbus connection broke"))?;
                    let wid = new_window.args()?.wid;
                    let this = self.clone();
                    tokio::spawn(async move {
                        if let Err(e) = this.map_win(wid).await {
                            info!("Failed to map window {}, {}", wid, e);
                        }
                    });
                }
                closed_window = win_unmapped.next() => {
                    let closed_window = closed_window.with_context(|| anyhow!("dbus connection broke"))?;
                    let this = self.clone();
                    let wid = closed_window.args()?.wid;
                    let mut window_state = this.window_state.write().await;
                    let w = window_state.windows.remove(&wid);
                    match w {
                        Some(w) =>  {
                            let w = w.into_inner();
                            window_state.client_window_to_window.remove(&w.client_wid);
                            drop(window_state);

                            // We have to remove window from window_state before handling any
                            // further events, so we don't get duplicated window error. That's why
                            // it is not part of this tokio::spawn
                            tokio::spawn(async move {
                                let window_state = this.window_state.write().await;
                                // window_state here is locked exclusively at this point.
                                unsafe { w.drop().await.unwrap() };
                                drop(window_state);
                            });
                        }
                        None => {},
                    }
                }
                input_event = input_rx.recv() => {
                    let input_event = input_event.unwrap();
                    let this = self.clone();
                    tokio::spawn(async move { this.handle_input_events(input_event).await });
                }
                _ = exit_rx.recv() => {
                    break;
                }
            }
        }
        Ok(())
    }

    pub fn drop(self) -> impl std::future::Future<Output = ()> {
        // Deconstruct self first, so the return future doesn't capture an App that might be
        // dropped
        let App {
            window_state,
            xrd_client,
            ..
        } = self;
        let window_state = window_state.into_inner();
        async {
            // Drop the Windows to defuse the drop bombs
            for (_, w) in window_state.windows.into_iter() {
                let w = w.into_inner();
                // We own window_state at this point
                unsafe { w.drop().await.unwrap() };
            }
            drop(xrd_client);
        }
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
            if width != win_geometry.width as u32 || height != win_geometry.height as u32 {
                info!("Free old textures for {}", wid);
                TextureSet::free(w.textures.take(), &self.gl, &self.x11).await?;
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
        if !w.xrd_window.get_mut().is_visible() {
            return Ok(());
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

        let xrd_window = w.xrd_window.get_mut();
        if refreshed {
            xrd_window.set_and_submit_texture(&textures.remote_texture);
        } else {
            xrd_window.submit_texture();
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
        debug!("Dbus connected {}", wid);
        if !proxy.mapped().await? {
            return Ok(());
        }
        let ty = proxy.type_().await?;
        debug!("window {} is {}", wid, ty);
        if ty != "normal"
            && ty != "menu"
            && ty != "popup_menu"
            && ty != "dropdown_menu"
            && ty != "utility"
        {
            return Ok(());
        }
        let window_name = proxy.name().await?;
        let client_wid = proxy.client_win().await?;
        let transient_for = block_in_place(|| {
            Result::Ok(
                self.x11
                    .get_property(
                        false,
                        wid,
                        self.atoms.WM_TRANSIENT_FOR,
                        xproto::AtomEnum::WINDOW,
                        0,
                        1,
                    )?
                    .reply()?,
            )
        })?
        .value32()
        .and_then(|mut w| w.next());
        debug!("transient for of {} is {:?}", wid, transient_for);
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
                gobject_sys::g_object_set(
                    xrd_window.as_object_ref().to_glib_none().0,
                    "native\0".as_bytes().as_ptr() as *const _,
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
        debug!("window created {}", wid);

        let (window_center_x, window_center_y) = (
            win_geometry.x + win_geometry.width as i16 / 2,
            win_geometry.y + win_geometry.height as i16 / 2,
        );

        {
            // Lock windows before xrd_client, because that's the order we used in render_win ->
            // refresh_texture.
            let window_state = self.window_state.read().await;
            let xrd_client = self.xrd_client.lock().await;
            let parent = if ty.contains("menu") || ty == "utility" {
                if let Some(leader) = transient_for {
                    Self::find_window_group(&window_state, leader).await
                } else {
                    let hovered = xrd_client.synth_hovered();
                    info!("no leader, hovered is {:?}", hovered);
                    hovered.map(|hovered| {
                        let mut native = 0u64;
                        unsafe {
                            gobject_sys::g_object_get(
                                hovered.as_ptr() as *mut _,
                                "native\0".as_bytes().as_ptr() as *const _,
                                &mut native as *mut _,
                                0,
                            );
                        }
                        info!("hovered is native {}", native);
                        window_state.windows.get(&(native as _)).unwrap()
                    })
                }
            } else {
                None
            };
            drop(xrd_client);
            if let Some(parent) = parent {
                let parent = parent.read().await;
                let parent_geometry =
                    block_in_place(|| Result::Ok(self.x11.get_geometry(parent.id)?.reply()?))?;
                let (parent_center_x, parent_center_y) = (
                    parent_geometry.x + parent_geometry.width as i16 / 2,
                    parent_geometry.y + parent_geometry.height as i16 / 2,
                );
                let mut offset = graphene::Point::new(
                    (window_center_x - parent_center_x) as _,
                    -(window_center_y - parent_center_y) as _,
                );
                parent
                    .xrd_window
                    .lock()
                    .await
                    .add_child(&xrd_window, &mut offset);
            } else {
                let point = graphene::Point3D::new(
                    (window_center_x - root_geometry.width as i16 / 2) as f32 / PIXELS_PER_METER,
                    -(window_center_y - root_geometry.height as i16 * 3 / 4) as f32
                        / PIXELS_PER_METER,
                    window_state.windows.len() as f32 / 3.0 - 8.0,
                );
                let mut transform = graphene::Matrix::new_translate(&point);
                xrd_window.set_transformation(&mut transform);
                xrd_window.set_reset_transformation(&mut transform);
            }
        }
        debug!("position set");

        let damage = self.x11.generate_id()?;
        let x11_clone = self.x11.clone();
        {
            let mut window_state = self.window_state.write().await;
            block_in_place(move || {
                x11_clone
                    .damage_create(damage, wid, x11rb::protocol::damage::ReportLevel::NON_EMPTY)?
                    .check()?;
                Result::Ok(())
            })?;

            let xrd_window = Mutex::new(xrd_window);
            let window = Window {
                id: wid,
                gl: self.gl.clone(),
                damage,
                x11: self.x11.clone(),
                xrd: self.xrd_client.clone(),
                textures: None,
                xrd_window,
                client_wid,
                drop_bomb: DropBomb::new("Window dropped unsafely"),
            };
            let parent_wid = window_state.client_window_to_window.insert(client_wid, wid);
            if let Some(parent_wid) = parent_wid {
                error!(
                    "Inconsistent window state, parent of {} was {}, updated to {}",
                    client_wid, parent_wid, wid
                );
            }
            debug!("inserting {}", wid);
            let window = match window_state.windows.try_insert(wid, RwLock::new(window)) {
                Ok(window) => window,
                Err(OccupiedError { value, .. }) => {
                    unsafe { value.into_inner().drop().await.unwrap() };
                    panic!("Duplicated window {}", wid);
                }
            };
            let mut window = window.write().await;
            self.render_win(&mut window).await?;
        }
        info!("Added new window {:#010x}", wid);
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
                            info!("Failed to map window {}, {}", wid, e);
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
#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() -> Result<()> {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(
        if cfg!(debug_assertions) {
            "app=debug"
        } else {
            "app=info "
        },
    ))
    .format_timestamp_millis()
    .init();
    if cfg!(debug_assertions) {
        std::env::set_var("G_DEBUG", "fatal-warnings");
        std::env::set_var("RUST_BACKTRACE", "1");
        std::env::set_var("VK_INSTANCE_LAYERS", "VK_LAYER_KHRONOS_validation");
    }
    let glib_mainloop = glib::MainLoop::new(None, false);
    let glib_mainloop2 = glib_mainloop.clone();
    let glib_thread = std::thread::spawn(move || {
        // Potential thread safety issue: the mainloop has references to xrdesktop objects, which
        // they might use concurrently with us, without locking.
        // An example is that it could call xrd_window_manager_poll_window_events on a window while
        // we are freeing it. There is no way to prevent this it seems.
        glib_mainloop2.run();
    });
    let ctx = Arc::new(App::new().await?);
    let result = ctx.clone().run().await;
    info!("App exited {:?}", result);

    // Stop glib mainloop
    glib_mainloop.quit();
    glib_thread.join().unwrap();

    // Must explicitly drop App
    let ctx = Arc::try_unwrap(ctx).unwrap();
    ctx.drop().await;
    result
}
