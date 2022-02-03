// Generated by gir (https://github.com/gtk-rs/gir @ e0d8d8d645b1)
// from ../gir-files (@ 54ae87ae2ece)
// from ../xrd-gir-files (@ 80772f94f710)
// DO NOT EDIT

use crate::Window;
use glib::object::IsA;
use glib::translate::*;
use std::mem;


#[doc(alias = "xrd_button_set_icon")]
pub fn button_set_icon(button: &impl IsA<Window>, client: &impl IsA<gulkan::Client>, upload_layout: u32, url: &str) {
    unsafe {
        ffi::xrd_button_set_icon(button.as_ref().to_glib_none().0, client.as_ref().to_glib_none().0, upload_layout, url.to_glib_none().0);
    }
}

//#[doc(alias = "xrd_math_clamp_towards_zero_2d")]
//pub fn math_clamp_towards_zero_2d(min: /*Ignored*/&mut graphene::Point, max: /*Ignored*/&mut graphene::Point, point: /*Ignored*/&mut graphene::Point, clamped: /*Ignored*/&mut graphene::Point) -> bool {
//    unsafe { TODO: call ffi:xrd_math_clamp_towards_zero_2d() }
//}

//#[doc(alias = "xrd_math_get_rotation_angles")]
//pub fn math_get_rotation_angles(direction: /*Ignored*/&mut graphene::Vec3, azimuth: f32, inclination: f32) {
//    unsafe { TODO: call ffi:xrd_math_get_rotation_angles() }
//}

//#[doc(alias = "xrd_math_intersect_lines_2d")]
//pub fn math_intersect_lines_2d(p0: /*Ignored*/&mut graphene::Point, p1: /*Ignored*/&mut graphene::Point, p2: /*Ignored*/&mut graphene::Point, p3: /*Ignored*/&mut graphene::Point, intersection: /*Ignored*/&mut graphene::Point) -> bool {
//    unsafe { TODO: call ffi:xrd_math_intersect_lines_2d() }
//}

#[doc(alias = "xrd_math_matrix_set_translation_point")]
pub fn math_matrix_set_translation_point(matrix: &mut graphene::Matrix, point: &mut graphene::Point3D) {
    unsafe {
        ffi::xrd_math_matrix_set_translation_point(matrix.to_glib_none_mut().0, point.to_glib_none_mut().0);
    }
}

//#[doc(alias = "xrd_math_matrix_set_translation_vec")]
//pub fn math_matrix_set_translation_vec(matrix: &mut graphene::Matrix, vec: /*Ignored*/&mut graphene::Vec3) {
//    unsafe { TODO: call ffi:xrd_math_matrix_set_translation_vec() }
//}

#[doc(alias = "xrd_math_point_matrix_distance")]
pub fn math_point_matrix_distance(intersection_point: &mut graphene::Point3D, pose: &mut graphene::Matrix) -> f32 {
    unsafe {
        ffi::xrd_math_point_matrix_distance(intersection_point.to_glib_none_mut().0, pose.to_glib_none_mut().0)
    }
}

/// ## `azimuth`
/// rotation around y axis, starting at -z. "left-right" component.
/// ## `inclination`
/// rotation upwards from xz plane. "up-down" component".
/// ## `distance`
/// the radius of the sphere
/// ## `point`
/// the resulting point in 3D space on the surface of a sphere around
/// (0,0,0) with `distance`.
#[doc(alias = "xrd_math_sphere_to_3d_coords")]
pub fn math_sphere_to_3d_coords(azimuth: f32, inclination: f32, distance: f32, point: &mut graphene::Point3D) {
    unsafe {
        ffi::xrd_math_sphere_to_3d_coords(azimuth, inclination, distance, point.to_glib_none_mut().0);
    }
}

#[doc(alias = "xrd_render_lock")]
pub fn render_lock() {
    unsafe {
        ffi::xrd_render_lock();
    }
}

#[doc(alias = "xrd_render_lock_destroy")]
pub fn render_lock_destroy() {
    unsafe {
        ffi::xrd_render_lock_destroy();
    }
}

#[doc(alias = "xrd_render_lock_init")]
pub fn render_lock_init() {
    unsafe {
        ffi::xrd_render_lock_init();
    }
}

#[doc(alias = "xrd_render_unlock")]
pub fn render_unlock() {
    unsafe {
        ffi::xrd_render_unlock();
    }
}

//#[doc(alias = "xrd_settings_connect_and_apply")]
//pub fn settings_connect_and_apply<P: FnOnce() + Send + Sync + 'static>(callback: P, key: &str, data: /*Unimplemented*/Option<Fundamental: Pointer>) {
//    unsafe { TODO: call ffi:xrd_settings_connect_and_apply() }
//}

#[doc(alias = "xrd_settings_destroy_instance")]
pub fn settings_destroy_instance() {
    unsafe {
        ffi::xrd_settings_destroy_instance();
    }
}

///
/// # Returns
///
/// The [`gio::Settings`][crate::gio::Settings] for xrdesktop
#[doc(alias = "xrd_settings_get_instance")]
pub fn settings_get_instance() -> Option<gio::Settings> {
    unsafe {
        from_glib_none(ffi::xrd_settings_get_instance())
    }
}

#[doc(alias = "xrd_settings_is_schema_installed")]
pub fn settings_is_schema_installed() -> bool {
    unsafe {
        from_glib(ffi::xrd_settings_is_schema_installed())
    }
}

/// Convencience callback that can be registered when no action is required on
/// an update of a double value.
/// ## `settings`
/// The gsettings
/// ## `key`
/// The key
///
/// # Returns
///
///
/// ## `val`
/// Pointer to a double value to be updated
#[doc(alias = "xrd_settings_update_double_val")]
pub fn settings_update_double_val(settings: &impl IsA<gio::Settings>, key: &str) -> f64 {
    unsafe {
        let mut val = mem::MaybeUninit::uninit();
        ffi::xrd_settings_update_double_val(settings.as_ref().to_glib_none().0, key.to_glib_none().0, val.as_mut_ptr());
        let val = val.assume_init();
        val
    }
}

/// Convencience callback that can be registered when no action is required on
/// an update of a gboolean value.
/// ## `settings`
/// The gsettings
/// ## `key`
/// The key
///
/// # Returns
///
///
/// ## `val`
/// Pointer to an gboolean value to be updated
#[doc(alias = "xrd_settings_update_gboolean_val")]
pub fn settings_update_gboolean_val(settings: &impl IsA<gio::Settings>, key: &str) -> bool {
    unsafe {
        let mut val = mem::MaybeUninit::uninit();
        ffi::xrd_settings_update_gboolean_val(settings.as_ref().to_glib_none().0, key.to_glib_none().0, val.as_mut_ptr());
        let val = val.assume_init();
        from_glib(val)
    }
}

/// Convencience callback that can be registered when no action is required on
/// an update of a int value.
/// ## `settings`
/// The gsettings
/// ## `key`
/// The key
///
/// # Returns
///
///
/// ## `val`
/// Pointer to an int value to be updated
#[doc(alias = "xrd_settings_update_int_val")]
pub fn settings_update_int_val(settings: &impl IsA<gio::Settings>, key: &str) -> i32 {
    unsafe {
        let mut val = mem::MaybeUninit::uninit();
        ffi::xrd_settings_update_int_val(settings.as_ref().to_glib_none().0, key.to_glib_none().0, val.as_mut_ptr());
        let val = val.assume_init();
        val
    }
}