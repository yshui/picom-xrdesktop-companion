// Generated by gir (https://github.com/gtk-rs/gir @ e0d8d8d645b1)
// from ../gir-files (@ 54ae87ae2ece)
// from ../xrd-gir-files (@ 8ccbb57ca024)
// DO NOT EDIT

use glib::translate::*;
use std::fmt;

#[derive(Debug, Eq, PartialEq, Ord, PartialOrd, Hash)]
#[derive(Clone, Copy)]
#[non_exhaustive]
#[doc(alias = "XrdClientMode")]
pub enum ClientMode {
    #[doc(alias = "XRD_CLIENT_MODE_OVERLAY")]
    Overlay,
    #[doc(alias = "XRD_CLIENT_MODE_SCENE")]
    Scene,
#[doc(hidden)]
    __Unknown(i32),
}

impl fmt::Display for ClientMode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "ClientMode::{}", match *self {
            Self::Overlay => "Overlay",
            Self::Scene => "Scene",
            _ => "Unknown",
        })
    }
}

#[doc(hidden)]
impl IntoGlib for ClientMode {
    type GlibType = ffi::XrdClientMode;

    fn into_glib(self) -> ffi::XrdClientMode {
        match self {
            Self::Overlay => ffi::XRD_CLIENT_MODE_OVERLAY,
            Self::Scene => ffi::XRD_CLIENT_MODE_SCENE,
            Self::__Unknown(value) => value,
}
    }
}

#[doc(hidden)]
impl FromGlib<ffi::XrdClientMode> for ClientMode {
    unsafe fn from_glib(value: ffi::XrdClientMode) -> Self {
        match value {
            ffi::XRD_CLIENT_MODE_OVERLAY => Self::Overlay,
            ffi::XRD_CLIENT_MODE_SCENE => Self::Scene,
            value => Self::__Unknown(value),
}
    }
}

