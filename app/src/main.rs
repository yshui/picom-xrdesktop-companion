use ::next_gen::prelude::*;
use std::{
    cell::RefCell,
    collections::{hash_map::Entry, HashMap},
    sync::{Arc, Weak},
};

use anyhow::{anyhow, Context};
use drop_bomb::DropBomb;
use futures::{StreamExt, TryStreamExt};
use gio::prelude::*;
use glib::{clone::Downgrade, translate::ToGlibPtr};
use gxr::ContextExt;
use log::*;
use tokio::{
    sync::{Mutex, RwLock},
    task::{block_in_place, spawn_blocking, JoinHandle},
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
mod input;
mod picom;
mod setup;
mod utils;

const PIXELS_PER_METER: f32 = 600.0;
type Result<T> = anyhow::Result<T>;

x11rb::atom_manager! {
    pub AtomCollection: AtomCollectionCookie {
        WM_TRANSIENT_FOR,
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
            block_in_place(|| Result::Ok(x11.free_pixmap(x11_pixmap)?.check()?))?;
            gl.release_texture(x11_texture).await?;
        }
        Ok(())
    }
    fn free_sync(this: Option<Self>, gl: &gl::Gl, x11: &RustConnection) -> Result<()> {
        if let Some(Self {
            x11_texture,
            x11_pixmap,
            ..
        }) = this
        {
            x11.free_pixmap(x11_pixmap)?.check()?;
            gl.release_texture_sync(x11_texture)?;
        }
        Ok(())
    }
}

#[derive(Debug)]
struct Window {
    id: xproto::Window,
    gl: gl::Gl,
    name: String,
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
    // Must be dropped with exclusive access to WindowState
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
    // Must either be dropped with exclusive access to WindowState
    unsafe fn drop_sync(self) -> Result<()> {
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
        let xrd = xrd.blocking_lock();
        xrd.remove_window(&xrd_window);
        xrd_window.close();
        // damage will have already been freed is window is closed
        // so ignore error
        x11.damage_destroy(damage).unwrap().ignore_error();
        TextureSet::free_sync(textures, &gl, &x11)
    }
}

#[derive(Debug)]
struct Cursor {
    texture: gulkan::Texture,
    width: u16,
    height: u16,
    hotspot_x: u32,
    hotspot_y: u32,
}

#[derive(Debug, Default)]
struct WindowState {
    windows: HashMap<u32, RwLock<Window>>,
    client_window_to_window: HashMap<u32, u32>,
}

struct App {
    gl: gl::Gl,
    dbus: zbus::Connection,
    xrd_client: Arc<Mutex<xrd::Client>>,
    input_synth: Mutex<inputsynth::InputSynth>,
    x11: Arc<RustConnection>,
    screen: u32,
    display: String,
    cursors: Mutex<std::collections::HashMap<u32, Cursor>>,
    atoms: AtomCollection,
    cursor_window: Mutex<xrd::Window>,
    /// What's the last position we set the cursor to?
    last_set_cursor: RwLock<Option<(u32, i16, i16)>>,
    window_state: RwLock<WindowState>,
    pending_windows: Mutex<HashMap<u32, JoinHandle<()>>>,
}

#[derive(Debug)]
enum InputEvent {
    /// Cursor moved by xrdesktop
    Move { wid: u32, x: f32, y: f32 },
    /// Click event from xrdesktop
    Click {
        wid: u32,
        x: f32,
        y: f32,
        button: xrd::sys::XrdInputSynthButton,
        pressed: bool,
    },
    /// Key presses from xrdesktop
    KeyPresses { string: Vec<i8> },
    /// Cursor moved by X11
    X11Move { wid: u32, x: i16, y: i16 },
}

impl std::fmt::Debug for App {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "App")
    }
}

impl Drop for App {
    fn drop(&mut self) {
        block_in_place(|| {
            let mut window_state = self.window_state.blocking_write();
            // Drop the Windows to defuse the drop bombs
            for (_, w) in window_state.windows.drain() {
                let w = w.into_inner();
                // We own window_state at this point
                unsafe { w.drop_sync().unwrap() };
            }
        })
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
        let input_synth =
            Mutex::new(inputsynth::InputSynth::new().expect("Failed to initialize inputsynth"));
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

        let cursor_window = xrd::Window::new_from_pixels(
            &client,
            "x11-cursor-mirror",
            (PIXELS_PER_METER / 10.) as u32,
            (PIXELS_PER_METER / 10.) as u32,
            PIXELS_PER_METER,
        )
        .ok_or_else(|| anyhow!("Failed to create cursor window"))?;
        unsafe {
            gobject_sys::g_object_set(
                cursor_window.as_object_ref().to_glib_none().0,
                "native\0".as_bytes().as_ptr() as *const _,
                u64::MAX as *const std::ffi::c_void,
                0,
            );
        }
        unsafe {
            xrd::sys::xrd_client_add_window(
                client.as_ptr(),
                cursor_window.as_ptr(),
                false as _,
                std::ptr::null_mut(),
            );
        }
        // We need to make sure cursor_window is hidden iff last_set_cursor is Some
        cursor_window.show();
        // Put cursor on top of everything
        cursor_window.set_sort_order(100);
        Ok(Self {
            gl: gl::Gl::new(x11.clone(), screen as u32).await?,
            dbus,
            window_state: Default::default(),
            xrd_client: Arc::new(Mutex::new(client)),
            input_synth,
            screen: screen as u32,
            x11,
            display: std::env::var("DISPLAY").unwrap().replace([':', '.'], "_"),
            cursors: Default::default(),
            atoms,
            last_set_cursor: Default::default(),
            cursor_window: cursor_window.into(),
            pending_windows: Default::default(),
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
                width: cursor_image.width,
                height: cursor_image.height,
                texture,
            })
        };
        let xrd_cursor = xrd_client.desktop_cursor().unwrap();
        xrd_cursor.set_and_submit_texture(cursor.texture.clone());
        xrd_cursor.set_hotspot(cursor.hotspot_x as _, cursor.hotspot_y as _);

        if cursor.width as f32 > PIXELS_PER_METER / 100.
            && cursor.height as f32 > PIXELS_PER_METER / 100.
        {
            // xrdesktop doesn't like windows smaller than 0.01 x 0.01
            let cursor_window = self.cursor_window.lock().await;
            cursor_window.set_and_submit_texture(cursor.texture.clone());
        }
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
                    // we receive a damage notify from X, but is yet to processed it;
                    // then we receive a WinUnmapped signal from picom, and we processed it;
                    // then we dequeue the damage notify from x11rb.
                    // this is not an error.
                    //
                    // problem caused by picom and X events are not synchronized.
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
            e => log::debug!("unhandled event: {:?}", e),
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
            Result::Ok((x, y, x - geometry.x, y - geometry.y))
        };

        let input_synth = self.input_synth.lock().await;
        let result =
            match input_event {
                InputEvent::Move { x, y, wid } => {
                    match raise_window_and_resolve_position(wid, x, y) {
                        Ok((x, y, x_off, y_off)) => {
                            if self.last_set_cursor.read().await.is_none() {
                                // We moved the cursor from xrdesktop, so hide the X11 cursor mirror
                                self.cursor_window.lock().await.hide();
                            }
                            *self.last_set_cursor.write().await = Some((wid, x_off, y_off));
                            // The window could have been closed, in that case we stop
                            input_synth.move_cursor(x, y).map_err(Into::into)
                        }
                        Err(e) => Err(e.into()),
                    }
                }

                InputEvent::Click {
                    x,
                    y,
                    wid,
                    button,
                    pressed,
                } => {
                    raise_window_and_resolve_position(wid, x, y).and_then(|(x, y, _, _)| {
                        // The window could have been closed, in that case we stop
                        input_synth
                            .click(x, y, button as _, pressed)
                            .map_err(Into::into)
                    })
                }
                InputEvent::KeyPresses { string } => {
                    debug!("key press {:?}", string);
                    block_in_place(|| {
                        string.into_iter().try_for_each(|ch| {
                            (if ch == b'\n' as _ {
                                input_synth.ascii_char(b'\r' as _)
                            } else {
                                input_synth.ascii_char(ch as _)
                            })
                            .map_err(Into::into)
                        })
                    })
                }
                InputEvent::X11Move { wid, x, y } => {
                    let last_set_cursor = *self.last_set_cursor.read().await;
                    if last_set_cursor == Some((wid, x, y)) {
                        // X11 reported back the cursor motion we synthesized, ignore it.
                        Ok(())
                    } else {
                        // TODO: user moved mouse in X, draw a cursor in VR for it.
                        let windows = self.window_state.read().await;
                        if let Some(window) = windows.windows.get(&wid) {
                            let window = window.read().await;
                            let Some((width, height)) = window
                                .textures
                                .as_ref()
                                .map(|ts| (ts.x11_texture.width(), ts.x11_texture.height()))
                            else {
                                return
                            };
                            // X11 is x to the right, y down, (0, 0) at top left
                            // xrdesktop is x to the right, y up, (0, 0) at center
                            let x = x as f32 / width as f32 - 0.5;
                            let y = 0.5 - y as f32 / height as f32;
                            log::info!("mouse: {x} {y}");
                            let mut transform = graphene::Matrix::new_identity();
                            let (width_meters, height_meters) = {
                                let xrd_window = window.xrd_window.lock().await;

                                if !xrd_window.is_transformation_no_scale(&mut transform) {
                                    return;
                                }
                                (
                                    xrd_window.current_width_meters(),
                                    xrd_window.current_height_meters(),
                                )
                            };
                            let translate = graphene::Matrix::new_translate(
                                &graphene::Point3D::new(x * width_meters, y * height_meters, 0.1),
                            );
                            let mut transform = translate.multiply(&transform);
                            log::info!("transform: {:?}, window: {}", transform, window.name);
                            {
                                let cursor_window = self.cursor_window.lock().await;
                                cursor_window.set_transformation(&mut transform);
                                cursor_window.set_reset_transformation(&mut transform);
                                if last_set_cursor.is_some() {
                                    // Show the X11 cursor mirror
                                    cursor_window.show();
                                }
                            }
                            *self.last_set_cursor.write().await = None;
                            Ok(())
                        } else {
                            // Not a window managed by us, ignore it.
                            log::warn!("X11 reported cursor move for unknown window {}", wid);
                            Ok(())
                        }
                    }
                }
            };
        if let Err(e) = result {
            error!("Failed to synthesis input {}", e);
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

    fn start_input_thread(self: Arc<Self>, input_tx: tokio::sync::mpsc::Sender<InputEvent>) {
        use ::input::{event::PointerEvent, Event};
        let root = self.x11.setup().roots[self.screen as usize].root;
        std::thread::spawn(move || -> Result<()> {
            let mut input = ::input::Libinput::new_with_udev(crate::input::Interface);
            input.udev_assign_seat("seat0").unwrap();
            loop {
                input.dispatch().unwrap();
                for event in &mut input {
                    match event {
                        Event::Pointer(PointerEvent::Motion(_)) => {
                            let mut current_window = root;
                            while current_window != x11rb::NONE {
                                // We don't care about the coordinates, we will query that from X
                                let pointer = self.x11.query_pointer(current_window)?.reply()?;
                                if self
                                    .window_state
                                    .blocking_read()
                                    .windows
                                    .get(&current_window)
                                    .is_some()
                                {
                                    input_tx.blocking_send(InputEvent::X11Move {
                                        wid: current_window,
                                        x: pointer.win_x,
                                        y: pointer.win_y,
                                    })?;
                                }
                                if current_window == pointer.child {
                                    break;
                                }
                                current_window = pointer.child;
                            }
                        }
                        _ => (),
                    }
                }
            }
        });
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

        // feature: never_type
        // tokio task for receiving X events
        spawn_blocking(move || -> Result<()> {
            loop {
                let event = x11_clone.wait_for_event()?;
                tx.blocking_send(event)?;
                while let Some(event) = x11_clone.poll_for_event()? {
                    tx.blocking_send(event)?;
                }
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
                if native != u64::MAX {
                    // If the queue is full, we drop the event
                    let _: std::result::Result<_, _> = tx.try_send(InputEvent::Move {
                        wid: native as u32,
                        x: point.x(),
                        y: point.y(),
                    });
                }
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
                if native != u64::MAX {
                    // We don't want to lose click events
                    let _ = tx.blocking_send(InputEvent::Click {
                        wid: native as u32,
                        x: point.x(),
                        y: point.y(),
                        button: event.button,
                        pressed: event.state != 0,
                    });
                }
            });
            let tx = input_tx.clone();
            xrd_client.connect_keyboard_press_event(move |_, event| {
                let event: &gdk::EventKey = event.downcast_ref().unwrap();
                let string = unsafe {
                    std::slice::from_raw_parts(event.as_ref().string, event.length() as _)
                };
                let string = string.to_owned();
                let _ = tx.blocking_send(InputEvent::KeyPresses { string });
            });
            self.clone().start_input_thread(input_tx);

            let (tx, exit_rx) = tokio::sync::mpsc::channel(1);
            xrd_client.connect_request_quit_event(move |_, reason| {
                if reason.reason == gxr::sys::GXR_QUIT_SHUTDOWN {
                    let _ = tx.blocking_send(*reason);
                }
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
                    debug!("{wid:#010x}, new window");
                    let this = self.clone();
                    let handle = tokio::spawn(async move {
                        if let Err(e) = this.map_win(wid).await {
                            info!("Failed to map window {}, {}", wid, e);
                        }
                    });
                    // feature: map_or_insert
                    match self.pending_windows.lock().await.entry(wid) {
                        Entry::Occupied(_) => {
                            panic!("Window {} already mapped", wid);
                        }
                        Entry::Vacant(entry) => {
                            entry.insert(handle);
                        }
                    }
                }
                closed_window = win_unmapped.next() => {
                    let closed_window = closed_window.with_context(|| anyhow!("dbus connection broke"))?;
                    let wid = closed_window.args()?.wid;
                    debug!("{wid:#010x} closed");
                    if let Some(handle) = self.pending_windows.lock().await.remove(&wid) {
                        debug!("stopped map_win task for {wid:#010x}");
                        handle.abort();
                        // we still need to continue, depending on the timing, map_win might have
                        // already inserted the window into window_state.
                    }
                    let this = self.clone();
                    let mut window_state = this.window_state.write().await;
                    let w = window_state.windows.remove(&wid);
                    if let Some(w) = w {
                        let w = w.into_inner();
                        window_state.client_window_to_window.remove(&w.client_wid);
                        drop(window_state);

                        // We have to remove window from window_state before handling any
                        // further events, so we wouldn't close a window with the same wid that
                        // is created _after_ we receive this event. That's why it is not part of
                        // tokio::spawn.
                        tokio::spawn(async move {
                            let window_state = this.window_state.write().await;
                            // window_state here is locked exclusively at this point.
                            unsafe { w.drop().await.unwrap() };
                            drop(window_state);
                            debug!("{wid:#010x} dropped");
                        });
                    }
                }
                input_event = input_rx.recv() => {
                    let input_event = input_event.unwrap();
                    let this = self.clone();
                    tokio::spawn(async move { this.handle_input_events(input_event).await });
                }
                exit = exit_rx.recv() => {
                    let exit = exit.with_context(|| anyhow!("exit channel broke"))?;
                    let xrd_client = self.xrd_client.lock().await;
                    let gxr = xrd_client.gxr_context().unwrap();
                    gxr.acknowledge_quit();
                    info!("Received exit request {:?}", exit);
                    break;
                }
            }
        }
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
            if width != win_geometry.width as u32 || height != win_geometry.height as u32 {
                debug!("Free old textures for {}", wid);
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
            xrd_window.set_and_submit_texture(textures.remote_texture.clone());
        } else {
            xrd_window.submit_texture();
        }
        Ok(())
    }

    async fn map_win_impl(&self, wid: u32) -> Result<()> {
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
                    wid as usize as *mut _,
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
                    debug!("no leader, hovered is {:?}", hovered);
                    hovered.and_then(|hovered| {
                        let mut native = 0u64;
                        unsafe {
                            gobject_sys::g_object_get(
                                hovered.as_ptr() as *mut _,
                                "native\0".as_bytes().as_ptr() as *const _,
                                &mut native as *mut _,
                                0,
                            );
                        }
                        debug!("hovered is native {}", native);
                        window_state.windows.get(&(native as _)) // would be None is hover is the
                                                                 // cursor window
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
            let win_attrs = block_in_place(move || {
                x11_clone
                    .damage_create(damage, wid, x11rb::protocol::damage::ReportLevel::NON_EMPTY)?
                    .check()?;
                Result::Ok(x11_clone.get_window_attributes(wid)?.reply()?)
            })?;

            // If we receive map -> unmap -> map event of the same window in quick
            // succession, the unmap event could be processed before the first map event (the
            // second map would never be processed before unmap OTOH). In that case that unmap
            // event will fail to remove any window and we have duplicated window. so:
            //
            // if we are unmapped currently, we give up adding this window. this is to prevent an
            // unmapped window from staying visible.
            //
            // if we are mapped, then we either didn't receive a unmap event, which is fine.
            // Otherwise, if we are the first map event, the window we are going to add here will
            // be replaced later by the second map event; if we are the second map event, we will
            // replace the existing window.
            if win_attrs.map_state != xproto::MapState::VIEWABLE {
                debug!("Window {wid:#010x} not viewable, giving up");
                return Ok(());
            }

            let xrd_window = Mutex::new(xrd_window);
            let window = Window {
                id: wid,
                name: window_name,
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
                if parent_wid != wid {
                    error!(
                        "Replaced parent of {client_wid:#010x}: was {parent_wid:#010x}, now to {wid:#010x}"
                    );
                    debug_assert!(false);
                }
            }
            debug!("inserting {}", wid);
            let mut entry = window_state.windows.entry(wid);
            let window = match entry {
                Entry::Vacant(entry) => entry.insert(RwLock::new(window)),
                Entry::Occupied(ref mut entry) => {
                    let old = entry.insert(RwLock::new(window));
                    // window_state is exclusively locked at this point, using the sync version
                    // so it couldn't be cancelled.
                    unsafe { block_in_place(|| old.into_inner().drop_sync().unwrap()) };
                    error!("Replaced old window entry for {wid:#010x}");
                    debug_assert!(false);
                    entry.get_mut()
                }
            };
            let mut window = window.write().await;
            self.render_win(&mut window).await?;
        }
        info!("Added new window {:#010x}", wid);
        //remove ourself from pending_windows
        Ok(())
    }

    async fn map_win(&self, wid: u32) -> Result<()> {
        let result = self.map_win_impl(wid).await;
        self.pending_windows.lock().await.remove(&wid);
        result
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
        Ok(futs.try_collect().await?)
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

type LockWheel = RefCell<Option<Pin<Box<dyn Generator<(), Return = (), Yield = ()>>>>>;
thread_local! {
    static LOCKWHEEL: LockWheel = RefCell::new(None);
    static OLD_POLL_FN: RefCell<glib_sys::GPollFunc> = RefCell::new(None);
}

// Returns a generator that alternatively locks/unlocks xrd_client and window_state.
// We use a generator because:
// 1. We need to store the lock guards in global variable, because glib's poll_func doesn't have
//    user data pointer.
// 2. Arc<App> must be store alongside because of lifetime requirements.
// 3. Because of 1 and 2, what we need to store is self-referential.
#[next_gen::generator(yield(()))]
fn locker(mut ctx: Weak<App>) {
    loop {
        ctx = if let Some(ctx) = ctx.upgrade() {
            let mut _window_state = ctx.window_state.blocking_write();
            let mut _xrd_client = ctx.xrd_client.blocking_lock();
            let mut _cursor_window = ctx.cursor_window.blocking_lock();
            yield_!(()); // Yield when locked
            ctx.downgrade()
        } else {
            break;
        };
        yield_!(()); // Yield when unlocked
    }

    // ctx has been dropped, there is no need to lock anything anymore.
    // yield in a loop to prevent lockwheel async fn from completing.
    loop {
        yield_!(());
    }
}
fn main() -> Result<()> {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(
        if cfg!(debug_assertions) {
            "app=debug"
        } else {
            "app=info "
        },
    ))
    .format_timestamp_millis()
    .init();
    setup::setup();
    let runtime = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(4)
        .enable_all()
        .build()
        .unwrap();
    if cfg!(debug_assertions) {
        std::env::set_var("G_DEBUG", "fatal-warnings");
        std::env::set_var("RUST_BACKTRACE", "1");
        std::env::set_var("VK_INSTANCE_LAYERS", "VK_LAYER_KHRONOS_validation");
    }
    let ctx = Arc::new(runtime.block_on(App::new())?);
    let ctx_weak = ctx.downgrade();

    let glib_mainloop = glib::MainLoop::new(None, false);
    let glib_mainloop2 = glib_mainloop.clone();
    let glib_thread = std::thread::spawn(move || {
        // Potential thread safety issue: the mainloop has references to xrdesktop objects, which
        // they might use concurrently with us, without locking.
        // An example is that it could call xrd_window_manager_poll_window_events on a window while
        // we are freeing it.

        // To prevent this, we hijack their poll() function. We unlock xrd_client and window_state
        // before blocking on poll() to not block the progress of App::run(), and lock them before
        // returning and giving control back to glib/xrdesktop.
        OLD_POLL_FN.with(|opf| {
            *opf.borrow_mut() = unsafe {
                glib_sys::g_main_context_get_poll_func(glib_mainloop2.context().to_glib_none().0)
            }
        });
        mk_gen!(let mut lock_wheel = box locker(ctx_weak););
        // Turn it once to lock it
        lock_wheel.as_mut().resume(());
        LOCKWHEEL.with(|lw| *lw.borrow_mut() = Some(lock_wheel as _));

        unsafe extern "C" fn glib_poll_func_trampoline(
            fd: *mut glib_sys::GPollFD,
            a: libc::c_uint,
            b: libc::c_int,
        ) -> libc::c_int {
            LOCKWHEEL.with(|lw| {
                // Unlock
                lw.borrow_mut().as_mut().unwrap().as_mut().resume(());
                let ret = OLD_POLL_FN.with(|opf| (opf.borrow().as_ref().unwrap())(fd, a, b));
                // Lock before return
                lw.borrow_mut().as_mut().unwrap().as_mut().resume(());
                trace!("lock wheel turned");
                ret
            })
        }
        unsafe {
            glib_sys::g_main_context_set_poll_func(
                glib_mainloop2.context().to_glib_none().0,
                Some(glib_poll_func_trampoline),
            )
        };

        // Running the glib mainloop is unsafe: we have to make sure App is locked and has a strong
        // reference before returning control to glib.
        // (OTOH dropping App is safe on its own)
        glib_mainloop2.run();

        // Drop LOCKWHEEL
        LOCKWHEEL.with(|lw| {
            lw.borrow_mut().take();
        });
    });
    let result = runtime.block_on(ctx.run());
    info!("App exited {:?}", result);

    // Stop glib mainloop
    glib_mainloop.quit();
    glib_thread.join().unwrap();

    // Wait for all tasks to finish
    drop(runtime);
    result
}
