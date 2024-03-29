// Generated by gir (https://github.com/gtk-rs/gir @ 2358cc24efd2)
// from ../gir-files (@ eab91ba8f88b)
// from ../xrd-gir-files (@ 82da2b8bb0f0+)
// DO NOT EDIT

use crate::Window;
use glib::{prelude::*, translate::*};
use std::mem;

#[doc(alias = "xrd_button_set_icon")]
pub fn button_set_icon(
    button: &impl IsA<Window>,
    client: &impl IsA<gulkan::Client>,
    upload_layout: u32,
    url: &str,
) {
    unsafe {
        ffi::xrd_button_set_icon(
            button.as_ref().to_glib_none().0,
            client.as_ref().to_glib_none().0,
            upload_layout,
            url.to_glib_none().0,
        );
    }
}

/// ## `min`
/// The (x,y) limit at the bottom left.
/// ## `max`
/// The (x,y) limit at the top right.
/// ## `point`
/// An (x,y) point, will be clamped if outside the min, max limits.
/// ## `clamped`
/// The clamped point, if the point was outside the limits.
///
/// # Returns
///
/// TRUE if the point was clamped, else FALSE.
///
/// The bottom left "min" and top right "max" limits define a rectangle.
/// Clamp a value to the borders of this rectangle such that both x and y go
/// towards zero, until a rectangle border is hit.
#[doc(alias = "xrd_math_clamp_towards_zero_2d")]
pub fn math_clamp_towards_zero_2d(
    min: &mut graphene::Point,
    max: &mut graphene::Point,
    point: &mut graphene::Point,
    clamped: &mut graphene::Point,
) -> bool {
    unsafe {
        from_glib(ffi::xrd_math_clamp_towards_zero_2d(
            min.to_glib_none_mut().0,
            max.to_glib_none_mut().0,
            point.to_glib_none_mut().0,
            clamped.to_glib_none_mut().0,
        ))
    }
}

//#[doc(alias = "xrd_math_get_rotation_angles")]
//pub fn math_get_rotation_angles(direction: /*Ignored*/&mut graphene::Vec3, azimuth: f32, inclination: f32) {
//    unsafe { TODO: call ffi:xrd_math_get_rotation_angles() }
//}

/// ## `p0`
/// The first point of the first line.
/// ## `p1`
/// The second point of the first line.
/// ## `p2`
/// The first point of the second line.
/// ## `p3`
/// The second point of the second line.
/// ## `intersection`
/// The resulting intersection point, if the lines intersect.
///
/// # Returns
///
/// TRUE if the lines intersect, else FALSE.
///
/// 2 lines are given by 2 consecutive (x,y) points each.
/// Based on an algorithm in Andre LeMothe's
/// "Tricks of the Windows Game Programming Gurus".
/// Implementation from https://stackoverflow.com/a/1968345
#[doc(alias = "xrd_math_intersect_lines_2d")]
pub fn math_intersect_lines_2d(
    p0: &mut graphene::Point,
    p1: &mut graphene::Point,
    p2: &mut graphene::Point,
    p3: &mut graphene::Point,
    intersection: &mut graphene::Point,
) -> bool {
    unsafe {
        from_glib(ffi::xrd_math_intersect_lines_2d(
            p0.to_glib_none_mut().0,
            p1.to_glib_none_mut().0,
            p2.to_glib_none_mut().0,
            p3.to_glib_none_mut().0,
            intersection.to_glib_none_mut().0,
        ))
    }
}

#[doc(alias = "xrd_math_matrix_set_translation_point")]
pub fn math_matrix_set_translation_point(
    matrix: &mut graphene::Matrix,
    point: &mut graphene::Point3D,
) {
    unsafe {
        ffi::xrd_math_matrix_set_translation_point(
            matrix.to_glib_none_mut().0,
            point.to_glib_none_mut().0,
        );
    }
}

//#[doc(alias = "xrd_math_matrix_set_translation_vec")]
//pub fn math_matrix_set_translation_vec(matrix: &mut graphene::Matrix, vec: /*Ignored*/&mut graphene::Vec3) {
//    unsafe { TODO: call ffi:xrd_math_matrix_set_translation_vec() }
//}

#[doc(alias = "xrd_math_point_matrix_distance")]
pub fn math_point_matrix_distance(
    intersection_point: &mut graphene::Point3D,
    pose: &mut graphene::Matrix,
) -> f32 {
    unsafe {
        ffi::xrd_math_point_matrix_distance(
            intersection_point.to_glib_none_mut().0,
            pose.to_glib_none_mut().0,
        )
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
pub fn math_sphere_to_3d_coords(
    azimuth: f32,
    inclination: f32,
    distance: f32,
    point: &mut graphene::Point3D,
) {
    unsafe {
        ffi::xrd_math_sphere_to_3d_coords(
            azimuth,
            inclination,
            distance,
            point.to_glib_none_mut().0,
        );
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
//pub fn settings_connect_and_apply<P: FnOnce() + Send + Sync + 'static>(callback: P, key: &str, data: /*Unimplemented*/Option<Basic: Pointer>) {
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
    unsafe { from_glib_none(ffi::xrd_settings_get_instance()) }
}

#[doc(alias = "xrd_settings_is_schema_installed")]
pub fn settings_is_schema_installed() -> bool {
    unsafe { from_glib(ffi::xrd_settings_is_schema_installed()) }
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
        ffi::xrd_settings_update_double_val(
            settings.as_ref().to_glib_none().0,
            key.to_glib_none().0,
            val.as_mut_ptr(),
        );
        val.assume_init()
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
        ffi::xrd_settings_update_gboolean_val(
            settings.as_ref().to_glib_none().0,
            key.to_glib_none().0,
            val.as_mut_ptr(),
        );
        from_glib(val.assume_init())
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
        ffi::xrd_settings_update_int_val(
            settings.as_ref().to_glib_none().0,
            key.to_glib_none().0,
            val.as_mut_ptr(),
        );
        val.assume_init()
    }
}
