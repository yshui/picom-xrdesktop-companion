#![feature(never_type, backtrace, map_try_insert, box_into_inner)]
use std::sync::Arc;

use anyhow::{anyhow, Context};
use gio::prelude::*;
use glib::translate::ToGlibPtr;
use libc::c_void;
use log::*;
use tokio::{sync::Mutex, task::spawn_blocking};
use x11rb::{
    connection::Connection,
    protocol::{
        composite::ConnectionExt as _,
        damage::{self, ConnectionExt as _},
        xproto::{self, ConnectionExt as _},
    },
    rust_connection::RustConnection,
};
use xrd::{ClientExt, WindowExt};

mod gl;
mod picom;
mod utils;

const PIXELS_PER_METER: f32 = 900.0;
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
            gl.release_texture(x11_texture).await?;
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
    textures: Option<TextureSet>,
    xrd_window: xrd::Window,
}

impl Drop for Window {
    fn drop(&mut self) {
        let textures = self.textures.take();
        let gl = self.gl.clone();
        let x11 = self.x11.clone();
        tokio::spawn(async move { TextureSet::free(textures, &gl, &x11).await.unwrap() });
    }
}

type WindowMap = Mutex<std::collections::HashMap<u32, Window>>;

struct App {
    gl: gl::Gl,
    dbus: zbus::Connection,
    windows: WindowMap,
    xrd_client: Mutex<xrd::Client>,
    input_synth: InputSynth,
    x11: Arc<RustConnection>,
    screen: u32,
    display: String,
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
        let input_synth = InputSynth::new().expect("Failed to initialize inputsynth");
        let (x11, screen) = RustConnection::connect(None)?;
        let x11 = Arc::new(x11);
        let (damage_major, damage_minor) = x11rb::protocol::damage::X11_XML_VERSION;
        x11.damage_query_version(damage_major, damage_minor)?
            .reply()?;
        Ok(Self {
            gl: gl::Gl::new(x11.clone(), screen as u32).await?,
            dbus,
            windows: Default::default(),
            xrd_client: Mutex::new(client),
            input_synth,
            screen: screen as u32,
            x11,
            display: "_0".to_owned(), //std::env::var("DISPLAY").unwrap().replace(':', "_"),
        })
    }

    async fn run(self: Arc<Self>) -> Result<!> {
        Self::setup_initial_windows(self.clone()).await?;
        loop {
            let x11_clone = self.x11.clone();
            let event = spawn_blocking(move || x11_clone.wait_for_event()).await??;
            trace!("{:?}", event);
            use x11rb::protocol::Event;
            match event {
                Event::DamageNotify(damage::NotifyEvent { drawable, .. }) => {
                    let mut windows = self.windows.lock().await;
                    let w = windows.get_mut(&drawable).unwrap();
                    let damage = w.damage;
                    let x11_clone = self.x11.clone();
                    spawn_blocking(move || {
                        Result::Ok(
                            x11_clone
                                .damage_subtract(damage, x11rb::NONE, x11rb::NONE)?
                                .check()?,
                        )
                    })
                    .await??;
                    self.render_win(w).await?;
                }
                _ => (),
            }
            //TODO: optimization: handle all queued events here using poll_for_event
        }
    }

    async fn refresh_texture(&self, w: &mut Window) -> Result<bool> {
        let x11_clone = self.x11.clone();
        let wid = w.id;
        let win_geometry =
            spawn_blocking(move || Result::Ok(x11_clone.as_ref().get_geometry(wid)?.reply()?))
                .await??;
        if let Some((width, height)) = w
            .textures
            .as_ref()
            .map(|ts| (ts.x11_texture.width(), ts.x11_texture.height()))
        {
            if width != win_geometry.width.into() || height != win_geometry.height.into() {
                info!("Free old textures for {}", wid);
                TextureSet::free(w.textures.take(), &self.gl, &self.x11).await?;
            }
        }

        if w.textures.is_none() {
            let x11_clone = self.x11.clone();
            let (attrs, x11_pixmap) = spawn_blocking(move || {
                let attrs = x11_clone.get_window_attributes(wid)?.reply()?;
                let x11_pixmap = x11_clone.generate_id()?;
                x11_clone
                    .composite_name_window_pixmap(wid, x11_pixmap)?
                    .check()?;
                Result::Ok((attrs, x11_pixmap))
            })
            .await??;
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

    async fn map_win(&self, wid: &str) -> Result<()> {
        info!("Beginning of map_win {}", wid);
        let picom_service = format!("com.github.chjj.compton.{}", self.display);
        let proxy = picom::WindowProxy::builder(&self.dbus)
            .destination(picom_service)?
            .path(format!("{}/{}/{}", PICOM_OBJECT_PATH, "windows", wid))
            .map(|pb| pb.cache_properties(zbus::CacheProperties::No))?
            .build()
            .await?;
        info!("dbus connected {}", wid);
        if !proxy.mapped().await? {
            return Ok(());
        }
        if proxy.type_().await? != "normal" {
            return Ok(());
        }
        let window_name = proxy.name().await?;
        info!("property read {}", wid);
        let wid: u32 = parse_int::parse(wid)?;
        // TODO: cache root geometry
        let root_win = self.x11.setup().roots[self.screen as usize].root;
        let x11_clone = self.x11.clone();
        let (root_geometry, win_geometry) = spawn_blocking(move || {
            Result::Ok((
                x11_clone.get_geometry(root_win)?.reply()?,
                x11_clone.get_geometry(wid)?.reply()?,
            ))
        })
        .await??;
        if win_geometry.x <= -(win_geometry.width as i16)
            || win_geometry.y <= -(win_geometry.height as i16)
            || win_geometry.x >= root_geometry.width as _
            || win_geometry.y >= root_geometry.height as _
        {
            // If the window is entirely outside of the screen, hide it
            // Firefox does this and has a 1x1 window outside the screen
            return Ok(());
        }
        info!("{}", wid);

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
        info!("xrd window created {}", wid);

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
        spawn_blocking(move || {
            x11_clone
                .damage_create(damage, wid, x11rb::protocol::damage::ReportLevel::NON_EMPTY)?
                .check()?;
            Result::Ok(())
        })
        .await??;

        let window = Window {
            id: wid,
            gl: self.gl.clone(),
            x11: self.x11.clone(),
            xrd_window,
            damage,
            textures: None,
        };
        let mut windows = self.windows.lock().await;
        let window = windows.try_insert(wid, window).unwrap();
        info!("before render_win {}", wid);
        self.render_win(window).await?;
        info!("after render_win {}", wid);
        Ok(())
    }

    async fn setup_initial_windows(self: Arc<Self>) -> Result<()> {
        let picom_service = format!("com.github.chjj.compton.{}", self.display);
        let proxy: zbus::Proxy<'_> = zbus::ProxyBuilder::new_bare(&self.dbus)
            .destination(picom_service)?
            .interface("what.ever")?
            .path(format!("{}/{}", PICOM_OBJECT_PATH, "windows"))?
            .build()
            .await?;

        let windows = zbus::xml::Node::from_reader(proxy.introspect().await?.as_bytes())?;
        for w in windows.nodes().into_iter() {
            let wid = w.name().unwrap();
            if let Err(e) = self.map_win(&wid).await {
                error!("Failed to map window {}, {}", wid, e);
            }
        }
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
    let ctx = Arc::new(App::new().await?);
    ctx.run().await?;
}
