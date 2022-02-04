#![allow(clippy::all)]
pub use auto::functions::*;
pub use auto::traits::*;
pub use auto::*;

pub use ffi as sys;

#[allow(unused_imports)]
mod auto;

pub trait ClientExtExt: ClientExt {
    #[doc(alias = "request-quit-event")]
    fn connect_request_quit_event<F: Fn(&Self, &gxr::sys::GxrQuitEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> glib::signal::SignalHandlerId;
}

impl<O: glib::IsA<auto::Client>> ClientExtExt for O {
    fn connect_request_quit_event<F: Fn(&Self, &gxr::sys::GxrQuitEvent) + Send + 'static>(
        &self,
        f: F,
    ) -> glib::signal::SignalHandlerId {
        use glib::object::Cast;
        use glib::translate::FromGlibPtrBorrow;
        unsafe extern "C" fn request_quit_event_trampoline<
            P: glib::IsA<Client>,
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
            glib::signal::connect_raw(
                self.as_ptr() as *mut _,
                b"request-quit-event\0".as_ptr() as *const _,
                Some(std::mem::transmute::<_, unsafe extern "C" fn()>(
                    request_quit_event_trampoline::<Self, F> as *const (),
                )),
                Box::into_raw(f),
            )
        }
    }
}
