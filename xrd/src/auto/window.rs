// Generated by gir (https://github.com/gtk-rs/gir @ 2358cc24efd2)
// from ../gir-files (@ eab91ba8f88b)
// from ../xrd-gir-files (@ 2f1ef32fd867)
// DO NOT EDIT

use glib::{prelude::*,signal::{connect_raw, SignalHandlerId},translate::*};
use std::{boxed::Box as Box_,fmt,mem::transmute};

glib::wrapper! {
    #[doc(alias = "XrdWindow")]
    pub struct Window(Object<ffi::XrdWindow, ffi::XrdWindowClass>);

    match fn {
        type_ => || ffi::xrd_window_get_type(),
    }
}

impl Window {
    //#[doc(alias = "xrd_window_new")]
    //pub fn new(g3k: /*Ignored*/&g3k::Context, title: &str, native: /*Unimplemented*/Option<Basic: Pointer>, size_pixels: /*Ignored*/&vulkan::Extent2D, size_meters: /*Ignored*/&mut graphene::Size) -> Window {
    //    unsafe { TODO: call ffi:xrd_window_new() }
    //}

    /// x axis points right, y axis points up.
    /// ## `child`
    /// An already existing window.
    /// window's center in pixels.
    #[doc(alias = "xrd_window_add_child")]
    pub fn add_child(&self, child: &Window) {
        unsafe {
            ffi::xrd_window_add_child(self.to_glib_none().0, child.to_glib_none().0);
        }
    }

    #[doc(alias = "xrd_window_close")]
    pub fn close(&self) {
        unsafe {
            ffi::xrd_window_close(self.to_glib_none().0);
        }
    }

    #[doc(alias = "xrd_window_get_parent")]
    #[doc(alias = "get_parent")]
#[must_use]
    pub fn parent(&self) -> Option<Window> {
        unsafe {
            from_glib_none(ffi::xrd_window_get_parent(self.to_glib_none().0))
        }
    }

    /// ## `intersection_3d`
    /// A [`graphene::Point3D`][crate::graphene::Point3D] intersection point in meters.
    /// ## `intersection_pixels`
    /// Intersection in object coordinates with origin at
    /// top-left in pixels.
    #[doc(alias = "xrd_window_get_pixel_intersection")]
    #[doc(alias = "get_pixel_intersection")]
    pub fn pixel_intersection(&self, intersection_3d: &mut graphene::Point3D, intersection_pixels: &mut graphene::Point) {
        unsafe {
            ffi::xrd_window_get_pixel_intersection(self.to_glib_none().0, intersection_3d.to_glib_none_mut().0, intersection_pixels.to_glib_none_mut().0);
        }
    }

    //#[doc(alias = "xrd_window_get_rect")]
    //#[doc(alias = "get_rect")]
    //pub fn rect(&self) -> /*Ignored*/Option<WindowRect> {
    //    unsafe { TODO: call ffi:xrd_window_get_rect() }
    //}

    #[doc(alias = "xrd_window_has_parent")]
    pub fn has_parent(&self) -> bool {
        unsafe {
            from_glib(ffi::xrd_window_has_parent(self.to_glib_none().0))
        }
    }

    #[doc(alias = "xrd_window_is_pinned")]
    pub fn is_pinned(&self) -> bool {
        unsafe {
            from_glib(ffi::xrd_window_is_pinned(self.to_glib_none().0))
        }
    }

    #[doc(alias = "xrd_window_is_selected")]
    pub fn is_selected(&self) -> bool {
        unsafe {
            from_glib(ffi::xrd_window_is_selected(self.to_glib_none().0))
        }
    }

    #[doc(alias = "xrd_window_reset_color")]
    pub fn reset_color(&self) {
        unsafe {
            ffi::xrd_window_reset_color(self.to_glib_none().0);
        }
    }

    //#[doc(alias = "xrd_window_set_color")]
    //pub fn set_color(&self, color: /*Ignored*/&graphene::Vec4) {
    //    unsafe { TODO: call ffi:xrd_window_set_color() }
    //}

    #[doc(alias = "xrd_window_set_flip_y")]
    pub fn set_flip_y(&self, flip_y: bool) {
        unsafe {
            ffi::xrd_window_set_flip_y(self.to_glib_none().0, flip_y.into_glib());
        }
    }

    /// ## `pinned`
    /// The pin status to set this window to
    /// ## `hide_unpinned`
    /// If TRUE, the window will be hidden if it is unpinned, and
    /// shown if it is pinned. This corresponds to the "show only pinned windows"
    /// mode set up in `XrdShell`.
    /// If FALSE, windows are always shown.
    /// Note that `hide_unpinned` only determines initial visibility, and does not
    /// keep track of further mode changes.
    #[doc(alias = "xrd_window_set_pin")]
    pub fn set_pin(&self, pinned: bool, hide_unpinned: bool) {
        unsafe {
            ffi::xrd_window_set_pin(self.to_glib_none().0, pinned.into_glib(), hide_unpinned.into_glib());
        }
    }

    //#[doc(alias = "xrd_window_set_rect")]
    //pub fn set_rect(&self, rect: /*Ignored*/&mut WindowRect) {
    //    unsafe { TODO: call ffi:xrd_window_set_rect() }
    //}

    #[doc(alias = "xrd_window_set_selection_color")]
    pub fn set_selection_color(&self, is_selected: bool) {
        unsafe {
            ffi::xrd_window_set_selection_color(self.to_glib_none().0, is_selected.into_glib());
        }
    }

    //pub fn native(&self) -> /*Unimplemented*/Basic: Pointer {
    //    glib::ObjectExt::property(self, "native")
    //}

    //pub fn set_native(&self, native: /*Unimplemented*/Basic: Pointer) {
    //    glib::ObjectExt::set_property(self,"native", native)
    //}

    pub fn title(&self) -> Option<glib::GString> {
        glib::ObjectExt::property(self, "title")
    }

    #[doc(alias = "button-press-event")]
    pub fn connect_button_press_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn button_press_event_trampoline<F: Fn(&Window, &gdk::Event) + Send + 'static>(this: *mut ffi::XrdWindow, object: *mut gdk::ffi::GdkEvent, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this), &from_glib_none(object))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"button-press-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(button_press_event_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }

    #[doc(alias = "button-release-event")]
    pub fn connect_button_release_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn button_release_event_trampoline<F: Fn(&Window, &gdk::Event) + Send + 'static>(this: *mut ffi::XrdWindow, object: *mut gdk::ffi::GdkEvent, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this), &from_glib_none(object))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"button-release-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(button_release_event_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }

    #[doc(alias = "destroy")]
    pub fn connect_destroy<F: Fn(&Self) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn destroy_trampoline<F: Fn(&Window) + Send + 'static>(this: *mut ffi::XrdWindow, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"destroy\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(destroy_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }

    #[doc(alias = "motion-notify-event")]
    pub fn connect_motion_notify_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn motion_notify_event_trampoline<F: Fn(&Window, &gdk::Event) + Send + 'static>(this: *mut ffi::XrdWindow, object: *mut gdk::ffi::GdkEvent, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this), &from_glib_none(object))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"motion-notify-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(motion_notify_event_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }

    #[doc(alias = "scroll-event")]
    pub fn connect_scroll_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn scroll_event_trampoline<F: Fn(&Window, &gdk::Event) + Send + 'static>(this: *mut ffi::XrdWindow, object: *mut gdk::ffi::GdkEvent, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this), &from_glib_none(object))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"scroll-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(scroll_event_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }

    #[doc(alias = "show")]
    pub fn connect_show<F: Fn(&Self) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn show_trampoline<F: Fn(&Window) + Send + 'static>(this: *mut ffi::XrdWindow, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"show\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(show_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }

    #[doc(alias = "native")]
    pub fn connect_native_notify<F: Fn(&Self) + Send + 'static>(&self, f: F) -> SignalHandlerId {
        unsafe extern "C" fn notify_native_trampoline<F: Fn(&Window) + Send + 'static>(this: *mut ffi::XrdWindow, _param_spec: glib::ffi::gpointer, f: glib::ffi::gpointer) {
            let f: &F = &*(f as *const F);
            f(&from_glib_borrow(this))
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(self.as_ptr() as *mut _, b"notify::native\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(notify_native_trampoline::<F> as *const ())), Box_::into_raw(f))
        }
    }
}

unsafe impl Send for Window {}

impl fmt::Display for Window {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.write_str("Window")
    }
}
