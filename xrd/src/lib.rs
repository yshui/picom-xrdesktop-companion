#![allow(clippy::all)]
use std::mem::transmute;

pub use auto::functions::*;
pub use auto::traits::*;
pub use auto::*;

pub use ffi as sys;

use glib::object::Cast;
use glib::signal::connect_raw;
use glib::signal::SignalHandlerId;
use glib::translate::FromGlibPtrBorrow;
use glib::IsA;
#[allow(unused_imports)]
mod auto;

pub trait ClientExtExt: ClientExt {
    #[doc(alias = "request-quit-event")]
    fn connect_request_quit_event<F: Fn(&Self, &gxr::sys::GxrQuitEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId;
    #[doc(alias = "click-event")]
    fn connect_click_event<F: Fn(&Self, &sys::XrdClickEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId;

    #[doc(alias = "move-cursor-event")]
    fn connect_move_cursor_event<F: Fn(&Self, &sys::XrdMoveCursorEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId;

    #[doc(alias = "keyboard-press-event")]
    fn connect_keyboard_press_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId;
}

impl<O> ClientExtExt for O
where
    O: IsA<auto::Client>,
{
    fn connect_click_event<F: Fn(&Self, &sys::XrdClickEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn request_quit_event_trampoline<
            P: IsA<Client>,
            F: Fn(&P, &sys::XrdClickEvent) + Send + 'static,
        >(
            this: *mut ffi::XrdClient,
            object: *mut sys::XrdClickEvent,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                auto::Client::from_glib_borrow(this).unsafe_cast_ref(),
                &*object,
            )
        }
        unsafe {
            let f: Box<F> = Box::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                b"click-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(
                    request_quit_event_trampoline::<Self, F> as *const (),
                )),
                Box::into_raw(f),
            )
        }
    }
    fn connect_move_cursor_event<F: Fn(&Self, &sys::XrdMoveCursorEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn request_quit_event_trampoline<
            P: IsA<Client>,
            F: Fn(&P, &sys::XrdMoveCursorEvent) + Send + 'static,
        >(
            this: *mut ffi::XrdClient,
            object: *mut sys::XrdMoveCursorEvent,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                auto::Client::from_glib_borrow(this).unsafe_cast_ref(),
                &*object,
            )
        }
        unsafe {
            let f: Box<F> = Box::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                b"move-cursor-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(
                    request_quit_event_trampoline::<Self, F> as *const (),
                )),
                Box::into_raw(f),
            )
        }
    }
    fn connect_request_quit_event<F: Fn(&Self, &gxr::sys::GxrQuitEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn request_quit_event_trampoline<
            P: IsA<Client>,
            F: Fn(&P, &gxr::sys::GxrQuitEvent) + Send + 'static,
        >(
            this: *mut ffi::XrdClient,
            object: *mut gxr::sys::GxrQuitEvent,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                auto::Client::from_glib_borrow(this).unsafe_cast_ref(),
                &*object,
            )
        }
        unsafe {
            let f: Box<F> = Box::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                b"request-quit-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(
                    request_quit_event_trampoline::<Self, F> as *const (),
                )),
                Box::into_raw(f),
            )
        }
    }

    fn connect_keyboard_press_event<F: Fn(&Self, &gdk::Event) + Send + 'static>(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn keyboard_press_event_trampoline<
            P: IsA<Client>,
            F: Fn(&P, &gdk::Event) + Send + 'static,
        >(
            this: *mut ffi::XrdClient,
            object: *mut gdk::ffi::GdkEvent,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                Client::from_glib_borrow(this).unsafe_cast_ref(),
                &gdk::Event::from_glib_borrow(object),
            )
        }
        unsafe {
            let f: Box<F> = Box::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                b"keyboard-press-event\0".as_ptr() as *const _,
                Some(transmute::<_, unsafe extern "C" fn()>(
                    keyboard_press_event_trampoline::<Self, F> as *const (),
                )),
                Box::into_raw(f),
            )
        }
    }
}
