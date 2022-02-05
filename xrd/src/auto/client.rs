// Generated by gir (https://github.com/gtk-rs/gir @ e0d8d8d645b1)
// from ../gir-files (@ 54ae87ae2ece)
// from ../xrd-gir-files (@ 9fa64cdbe08c)
// DO NOT EDIT

use crate::ClientMode;
use crate::Container;
use crate::DesktopCursor;
use crate::InputSynth;
use crate::Window;
use crate::WindowManager;
use glib::object::Cast;
use glib::object::IsA;
use glib::signal::connect_raw;
use glib::signal::SignalHandlerId;
use glib::translate::*;
use std::boxed::Box as Box_;
use std::fmt;
use std::mem::transmute;

glib::wrapper! {
    #[doc(alias = "XrdClient")]
    pub struct Client(Object<ffi::XrdClient, ffi::XrdClientClass>);

    match fn {
        type_ => || ffi::xrd_client_get_type(),
    }
}

impl Client {
        pub const NONE: Option<&'static Client> = None;
    

    #[doc(alias = "xrd_client_new")]
    pub fn new() -> Client {
        unsafe {
            from_glib_full(ffi::xrd_client_new())
        }
    }

    #[doc(alias = "xrd_client_new_with_mode")]
    #[doc(alias = "new_with_mode")]
    pub fn with_mode(mode: ClientMode) -> Client {
        unsafe {
            from_glib_full(ffi::xrd_client_new_with_mode(mode.into_glib()))
        }
    }
}

impl Default for Client {
                     fn default() -> Self {
                         Self::new()
                     }
                 }

unsafe impl Send for Client {}

/// Trait containing all [`struct@Client`] methods.
///
/// # Implementors
///
/// [`Client`][struct@crate::Client]
pub trait ClientExt: 'static {
    //#[doc(alias = "xrd_client_add_button")]
    //fn add_button<P: FnOnce() + Send + 'static>(&self, button: &impl IsA<Window>, position: &mut graphene::Point3D, press_callback: P, press_callback_data: /*Unimplemented*/Option<Fundamental: Pointer>);

    /// For a container to start behaving according to its layout and attachment,
    /// it must be added to the client.
    ///
    /// Note: windows in the container must be added to the client separately with
    /// `xrd_client_add_window()`, preferably with draggable set to FALSE.
    /// ## `container`
    /// The [`Container`][crate::Container] to add
    #[doc(alias = "xrd_client_add_container")]
    fn add_container(&self, container: &Container);

    //#[doc(alias = "xrd_client_add_window")]
    //fn add_window(&self, window: &impl IsA<Window>, draggable: bool, lookup_key: /*Unimplemented*/Option<Fundamental: Pointer>);

    /// Creates a button and submits a Cairo rendered image to it.
    /// ## `width`
    /// Width in meters.
    /// ## `height`
    /// Height in meters.
    /// ## `ppm`
    /// Density in pixels per meter
    /// ## `url`
    /// The url of the icon
    ///
    /// # Returns
    ///
    /// a new [`Window`][crate::Window] representing the button
    #[doc(alias = "xrd_client_button_new_from_icon")]
    fn button_new_from_icon(&self, width: f32, height: f32, ppm: f32, url: &str) -> Option<Window>;

    /// Creates a button and submits a Cairo rendered text label to it.
    /// ## `width`
    /// Width in meters.
    /// ## `height`
    /// Height in meters.
    /// ## `ppm`
    /// Density in pixels per meter
    /// ## `label`
    /// One or more lines of text that will be
    /// displayed on the button.
    ///
    /// # Returns
    ///
    /// a new [`Window`][crate::Window] representing the button
    #[doc(alias = "xrd_client_button_new_from_text")]
    fn button_new_from_text(&self, width: f32, height: f32, ppm: f32, label: &[&str]) -> Option<Window>;

    ///
    /// # Returns
    ///
    /// A list of [`gxr::Controller`][crate::gxr::Controller].
    #[doc(alias = "xrd_client_get_controllers")]
    #[doc(alias = "get_controllers")]
    fn controllers(&self) -> Vec<gxr::Controller>;

    ///
    /// # Returns
    ///
    /// The [`DesktopCursor`][crate::DesktopCursor].
    #[doc(alias = "xrd_client_get_desktop_cursor")]
    #[doc(alias = "get_desktop_cursor")]
    fn desktop_cursor(&self) -> Option<DesktopCursor>;

    ///
    /// # Returns
    ///
    /// The [`gulkan::Client`][crate::gulkan::Client].
    #[doc(alias = "xrd_client_get_gulkan")]
    #[doc(alias = "get_gulkan")]
    fn gulkan(&self) -> Option<gulkan::Client>;

    ///
    /// # Returns
    ///
    /// The [`gxr::Context`][crate::gxr::Context]
    #[doc(alias = "xrd_client_get_gxr_context")]
    #[doc(alias = "get_gxr_context")]
    fn gxr_context(&self) -> Option<gxr::Context>;

    ///
    /// # Returns
    ///
    /// A [`InputSynth`][crate::InputSynth].
    #[doc(alias = "xrd_client_get_input_synth")]
    #[doc(alias = "get_input_synth")]
    fn input_synth(&self) -> Option<InputSynth>;

    ///
    /// # Returns
    ///
    /// The window that is currently used for
    /// keyboard input. Can be [`None`].
    #[doc(alias = "xrd_client_get_keyboard_window")]
    #[doc(alias = "get_keyboard_window")]
    fn keyboard_window(&self) -> Option<Window>;

    ///
    /// # Returns
    ///
    /// A [`WindowManager`][crate::WindowManager].
    #[doc(alias = "xrd_client_get_manager")]
    #[doc(alias = "get_manager")]
    fn manager(&self) -> Option<WindowManager>;

    ///
    /// # Returns
    ///
    /// If the controller used for synthesizing
    /// input is hovering over an [`Window`][crate::Window], return this window, else [`None`].
    #[doc(alias = "xrd_client_get_synth_hovered")]
    #[doc(alias = "get_synth_hovered")]
    fn synth_hovered(&self) -> Option<Window>;

    ///
    /// # Returns
    ///
    /// an `VkImageLayout`
    #[doc(alias = "xrd_client_get_upload_layout")]
    #[doc(alias = "get_upload_layout")]
    fn upload_layout(&self) -> u32;

    ///
    /// # Returns
    ///
    /// A list of [`Window`][crate::Window].
    #[doc(alias = "xrd_client_get_windows")]
    #[doc(alias = "get_windows")]
    fn windows(&self) -> Vec<Window>;

    //#[doc(alias = "xrd_client_lookup_window")]
    //fn lookup_window(&self, key: /*Unimplemented*/Option<Fundamental: Pointer>) -> Option<Window>;

    #[doc(alias = "xrd_client_poll_input_events")]
    fn poll_input_events(&self) -> bool;

    #[doc(alias = "xrd_client_poll_runtime_events")]
    fn poll_runtime_events(&self) -> bool;

    #[doc(alias = "xrd_client_remove_container")]
    fn remove_container(&self, container: &Container);

    /// Removes an [`Window`][crate::Window] from the management of the [`Client`][crate::Client] and the
    /// [`WindowManager`][crate::WindowManager].
    /// Note that the [`Window`][crate::Window] will not be destroyed by this function.
    /// ## `window`
    /// The [`Window`][crate::Window] to remove.
    #[doc(alias = "xrd_client_remove_window")]
    fn remove_window(&self, window: &impl IsA<Window>);

    #[doc(alias = "xrd_client_set_pin")]
    fn set_pin(&self, win: &impl IsA<Window>, pin: bool);

    #[doc(alias = "xrd_client_show_pinned_only")]
    fn show_pinned_only(&self, pinned_only: bool);

    /// References to gulkan, gxr and xrdesktop objects (like [`Window`][crate::Window])
    /// will be invalid after calling this function.
    ///
    /// [`switch_mode()`][Self::switch_mode()] replaces each [`Window`][crate::Window] with an appropriate new
    /// one, preserving its transformation matrix, scaling, pinned status, etc.
    ///
    /// The caller is responsible for reconnecting callbacks to [`Client`][crate::Client] signals.
    /// The caller is responsible to not use references to any previous [`Window`][crate::Window].
    /// Pointers to `XrdWindowData` will remain valid, however
    /// `XrdWindowData`->xrd_window will point to a new [`Window`][crate::Window].
    ///
    /// # Returns
    ///
    /// A new [`Client`][crate::Client] of the opposite
    /// mode than the passed one.
    #[doc(alias = "xrd_client_switch_mode")]
#[must_use]
    fn switch_mode(&self) -> Option<Client>;

    #[doc(alias = "keyboard-press-event")]
    fn connect_keyboard_press_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(&self, f: F) -> SignalHandlerId;

    //#[doc(alias = "request-quit-event")]
    //fn connect_request_quit_event<Unsupported or ignored types>(&self, f: F) -> SignalHandlerId;
}

impl<O: IsA<Client>> ClientExt for O {
    //fn add_button<P: FnOnce() + Send + 'static>(&self, button: &impl IsA<Window>, position: &mut graphene::Point3D, press_callback: P, press_callback_data: /*Unimplemented*/Option<Fundamental: Pointer>) {
    //    unsafe { TODO: call ffi:xrd_client_add_button() }
    //}

    fn add_container(&self, container: &Container) {
        unsafe {
            ffi::xrd_client_add_container(self.as_ref().to_glib_none().0, container.to_glib_none().0);
        }
    }

    //fn add_window(&self, window: &impl IsA<Window>, draggable: bool, lookup_key: /*Unimplemented*/Option<Fundamental: Pointer>) {
    //    unsafe { TODO: call ffi:xrd_client_add_window() }
    //}

    fn button_new_from_icon(&self, width: f32, height: f32, ppm: f32, url: &str) -> Option<Window> {
        unsafe {
            from_glib_full(ffi::xrd_client_button_new_from_icon(self.as_ref().to_glib_none().0, width, height, ppm, url.to_glib_none().0))
        }
    }

    fn button_new_from_text(&self, width: f32, height: f32, ppm: f32, label: &[&str]) -> Option<Window> {
        let label_count = label.len() as i32;
        unsafe {
            from_glib_full(ffi::xrd_client_button_new_from_text(self.as_ref().to_glib_none().0, width, height, ppm, label_count, label.to_glib_none().0))
        }
    }

    fn controllers(&self) -> Vec<gxr::Controller> {
        unsafe {
            FromGlibPtrContainer::from_glib_none(ffi::xrd_client_get_controllers(self.as_ref().to_glib_none().0))
        }
    }

    fn desktop_cursor(&self) -> Option<DesktopCursor> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_desktop_cursor(self.as_ref().to_glib_none().0))
        }
    }

    fn gulkan(&self) -> Option<gulkan::Client> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_gulkan(self.as_ref().to_glib_none().0))
        }
    }

    fn gxr_context(&self) -> Option<gxr::Context> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_gxr_context(self.as_ref().to_glib_none().0))
        }
    }

    fn input_synth(&self) -> Option<InputSynth> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_input_synth(self.as_ref().to_glib_none().0))
        }
    }

    fn keyboard_window(&self) -> Option<Window> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_keyboard_window(self.as_ref().to_glib_none().0))
        }
    }

    fn manager(&self) -> Option<WindowManager> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_manager(self.as_ref().to_glib_none().0))
        }
    }

    fn synth_hovered(&self) -> Option<Window> {
        unsafe {
            from_glib_none(ffi::xrd_client_get_synth_hovered(self.as_ref().to_glib_none().0))
        }
    }

    fn upload_layout(&self) -> u32 {
        unsafe {
            ffi::xrd_client_get_upload_layout(self.as_ref().to_glib_none().0)
        }
    }

    fn windows(&self) -> Vec<Window> {
        unsafe {
            FromGlibPtrContainer::from_glib_none(ffi::xrd_client_get_windows(self.as_ref().to_glib_none().0))
        }
    }

    //fn lookup_window(&self, key: /*Unimplemented*/Option<Fundamental: Pointer>) -> Option<Window> {
    //    unsafe { TODO: call ffi:xrd_client_lookup_window() }
    //}

    fn poll_input_events(&self) -> bool {
        unsafe {
            from_glib(ffi::xrd_client_poll_input_events(self.as_ref().to_glib_none().0))
        }
    }

    fn poll_runtime_events(&self) -> bool {
        unsafe {
            from_glib(ffi::xrd_client_poll_runtime_events(self.as_ref().to_glib_none().0))
        }
    }

    fn remove_container(&self, container: &Container) {
        unsafe {
            ffi::xrd_client_remove_container(self.as_ref().to_glib_none().0, container.to_glib_none().0);
        }
    }

    fn remove_window(&self, window: &impl IsA<Window>) {
        unsafe {
            ffi::xrd_client_remove_window(self.as_ref().to_glib_none().0, window.as_ref().to_glib_none().0);
        }
    }

    fn set_pin(&self, win: &impl IsA<Window>, pin: bool) {
        unsafe {
            ffi::xrd_client_set_pin(self.as_ref().to_glib_none().0, win.as_ref().to_glib_none().0, pin.into_glib());
        }
    }

    fn show_pinned_only(&self, pinned_only: bool) {
        unsafe {
            ffi::xrd_client_show_pinned_only(self.as_ref().to_glib_none().0, pinned_only.into_glib());
        }
    }

    fn switch_mode(&self) -> Option<Client> {
        unsafe {
            from_glib_full(ffi::xrd_client_switch_mode(self.as_ref().to_glib_full()))
        }
    }

    fn connect_keyboard_press_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn keyboard_press_event_trampoline<P: IsA<Client>, F: Fn(&P, &gdk::Event) + Send + 'static>(this: *mut ffi::XrdClient, object: *mut gdk::ffi::GdkEvent, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(Client::from_glib_borrow(this).unsafe_cast_ref(), &from_glib_none(object))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"keyboard-press-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(keyboard_press_event_trampoline::<Self, F> as *const ())), Box_::into_raw(f))
        }
    }

    //fn connect_request_quit_event<Unsupported or ignored types>(&self, f: F) -> SignalHandlerId {
    //    Unimplemented object: *.Pointer
    //}
}

impl fmt::Display for Client {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.write_str("Client")
    }
}
