/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-overlay-window-private.h"

#include <glib/gprintf.h>

#include "graphene-ext.h"
#include "xrd-math.h"
#include <gxr.h>

struct _XrdOverlayWindow {
	GObject parent;
	gboolean recreate;

	GxrOverlay *overlay;

	XrdWindowData *window_data;
};

enum {
	PROP_TITLE = 1,
	PROP_SCALE,
	PROP_NATIVE,
	PROP_TEXTURE_WIDTH,
	PROP_TEXTURE_HEIGHT,
	PROP_WIDTH_METERS,
	PROP_HEIGHT_METERS,
	N_PROPERTIES
};

static void xrd_overlay_window_window_interface_init(XrdWindowInterface *iface);

G_DEFINE_TYPE_WITH_CODE(
    XrdOverlayWindow, xrd_overlay_window, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(XRD_TYPE_WINDOW, xrd_overlay_window_window_interface_init))

static void xrd_overlay_window_set_property(GObject *object, guint property_id,
                                            const GValue *value, GParamSpec *pspec) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(object);
	switch (property_id) {
	case PROP_TITLE:
		if (self->window_data->title)
			g_string_free(self->window_data->title, TRUE);
		self->window_data->title = g_string_new(g_value_get_string(value));
		break;
	case PROP_SCALE:
		self->window_data->scale = g_value_get_float(value);
		break;
	case PROP_NATIVE:
		self->window_data->native = g_value_get_pointer(value);
		break;
	case PROP_TEXTURE_WIDTH:
		self->window_data->texture_width = g_value_get_uint(value);
		break;
	case PROP_TEXTURE_HEIGHT:
		self->window_data->texture_height = g_value_get_uint(value);
		break;
	case PROP_WIDTH_METERS:
		self->window_data->initial_size_meters.x = g_value_get_float(value);
		break;
	case PROP_HEIGHT_METERS:
		self->window_data->initial_size_meters.y = g_value_get_float(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void xrd_overlay_window_get_property(GObject *object, guint property_id,
                                            GValue *value, GParamSpec *pspec) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(object);

	switch (property_id) {
	case PROP_TITLE:
		g_value_set_string(value, self->window_data->title->str);
		break;
	case PROP_SCALE:
		g_value_set_float(value, self->window_data->scale);
		break;
	case PROP_NATIVE:
		g_value_set_pointer(value, self->window_data->native);
		break;
	case PROP_TEXTURE_WIDTH:
		g_value_set_uint(value, self->window_data->texture_width);
		break;
	case PROP_TEXTURE_HEIGHT:
		g_value_set_uint(value, self->window_data->texture_height);
		break;
	case PROP_WIDTH_METERS:
		g_value_set_float(value, self->window_data->initial_size_meters.x);
		break;
	case PROP_HEIGHT_METERS:
		g_value_set_float(value, self->window_data->initial_size_meters.y);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _update_dimensions(XrdOverlayWindow *self) {
	float width_meters = xrd_window_get_current_width_meters(XRD_WINDOW(self));
	gxr_overlay_set_width_meters(self->overlay, width_meters);

	uint32_t w, h;
	g_object_get(self, "texture-width", &w, "texture-height", &h, NULL);

	gxr_overlay_set_mouse_scale(self->overlay, (float)w, (float)h);

	if (self->window_data->child_window)
		xrd_window_update_child(XRD_WINDOW(self));
}

static void
_update_dimensions_cb(GObject *object, GParamSpec *pspec, gpointer user_data) {
	(void)pspec;
	(void)user_data;

	_update_dimensions(XRD_OVERLAY_WINDOW(object));
}

static void xrd_overlay_window_finalize(GObject *gobject);

static void xrd_overlay_window_constructed(GObject *gobject);

static void xrd_overlay_window_class_init(XrdOverlayWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = xrd_overlay_window_finalize;
	object_class->constructed = xrd_overlay_window_constructed;

	object_class->set_property = xrd_overlay_window_set_property;
	object_class->get_property = xrd_overlay_window_get_property;

	g_object_class_override_property(object_class, PROP_TITLE, "title");
	g_object_class_override_property(object_class, PROP_SCALE, "scale");
	g_object_class_override_property(object_class, PROP_NATIVE, "native");
	g_object_class_override_property(object_class, PROP_TEXTURE_WIDTH,
	                                 "texture-width");
	g_object_class_override_property(object_class, PROP_TEXTURE_HEIGHT,
	                                 "texture-height");
	g_object_class_override_property(object_class, PROP_WIDTH_METERS,
	                                 "initial-width-meters");
	g_object_class_override_property(object_class, PROP_HEIGHT_METERS,
	                                 "initial-height-meters");
}

static gboolean _set_transformation(XrdWindow *window, graphene_matrix_t *mat) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	gboolean res = gxr_overlay_set_transform_absolute(self->overlay, mat);
	if (self->window_data->child_window)
		xrd_window_update_child(window);

	XrdWindowData *data = xrd_window_get_data(window);
	graphene_matrix_t transform_unscaled;
	xrd_window_get_transformation_no_scale(window, &transform_unscaled);
	graphene_matrix_init_from_matrix(&data->transform, &transform_unscaled);

	return res;
}

static gboolean _get_transformation(XrdWindow *window, graphene_matrix_t *mat) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);

	graphene_matrix_t mat_no_scale;
	if (!gxr_overlay_get_transform_absolute(self->overlay, &mat_no_scale))
		return FALSE;

	/* Rebuild model matrix to include scale */
	float width_meters;
	if (!gxr_overlay_get_width_meters(self->overlay, &width_meters))
		return FALSE;

	float height_meters = width_meters / xrd_window_get_aspect_ratio(window);

	graphene_matrix_init_scale(mat, height_meters, height_meters, height_meters);

	graphene_quaternion_t orientation;
	graphene_ext_matrix_get_rotation_quaternion(&mat_no_scale, &orientation);
	graphene_matrix_rotate_quaternion(mat, &orientation);

	graphene_point3d_t position;
	graphene_ext_matrix_get_translation_point3d(&mat_no_scale, &position);
	graphene_matrix_translate(mat, &position);

	return TRUE;
}

static gboolean
_get_transformation_no_scale(XrdWindow *window, graphene_matrix_t *mat) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	if (!gxr_overlay_get_transform_absolute(self->overlay, mat))
		return FALSE;

	return TRUE;
}

static void _submit_texture(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);

	gxr_overlay_submit_texture(self->overlay, self->window_data->texture);
}

static void _set_and_submit_texture(XrdWindow *window, GulkanTexture *texture) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);

	uint32_t current_width, current_height;
	g_object_get(self, "texture-width", &current_width, "texture-height",
	             &current_height, NULL);

	VkExtent2D new_extent = gulkan_texture_get_extent(texture);

	/* update overlay if there is no texture, even if the texture dims
	 * are already the same */
	if (!self->window_data->texture || current_width != new_extent.width ||
	    current_height != new_extent.height) {
		/* initial-dims are respective the texture size and ppm.
		 * Now that the texture size changed, initial dims need to be
		 * updated, using the original ppm used to create this window. */
		float initial_width_meter, initial_height_meter;
		g_object_get(self, "initial-width-meters", &initial_width_meter,
		             "initial-height-meters", &initial_height_meter, NULL);

		double original_ppm =
		    (double)current_width / (double)initial_width_meter;
		double new_initial_width_meter =
		    (double)new_extent.width / original_ppm;
		double new_initial_height_meter =
		    (double)new_extent.height / original_ppm;

		g_object_set(self, "initial-width-meters", new_initial_width_meter,
		             "initial-height-meters", new_initial_height_meter, NULL);

		g_object_set(self, "texture-width", new_extent.width,
		             "texture-height", new_extent.height, NULL);

		float width_meters =
		    xrd_window_get_current_width_meters(XRD_WINDOW(self));

		gxr_overlay_set_width_meters(self->overlay, width_meters);

		/* Mouse scale is required for the intersection test */
		gxr_overlay_set_mouse_scale(self->overlay, (float)new_extent.width,
		                            (float)new_extent.height);
	}

	/* let the previous texture stay alive until this one has been submitted */
	GulkanTexture *to_free = self->window_data->texture;
	self->window_data->texture = texture;

	_submit_texture(window);

	if (to_free)
		g_object_unref(to_free);
}

static GulkanTexture *_get_texture(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	return self->window_data->texture;
}

static void
_add_child(XrdWindow *window, XrdWindow *child, graphene_point_t *offset_center) {
	(void)window;
	(void)offset_center;
	/* TODO: sort order hierarchy instead od ad hoc values*/
	XrdOverlayWindow *child_overlay = XRD_OVERLAY_WINDOW(child);
	gxr_overlay_set_sort_order(child_overlay->overlay, 1);
}

static void _poll_event(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	gxr_overlay_poll_event(self->overlay);
}

static void xrd_overlay_window_init(XrdOverlayWindow *self) {
	self->window_data = g_malloc(sizeof(XrdWindowData));
	self->window_data->title = NULL;
	self->window_data->child_window = NULL;
	self->window_data->parent_window = NULL;
	self->window_data->native = NULL;
	self->window_data->texture_width = 0;
	self->window_data->texture_height = 0;
	self->window_data->texture = NULL;
	self->window_data->xrd_window = XRD_WINDOW(self);
	self->window_data->pinned = FALSE;

	self->window_data->owned_by_window = TRUE;

	graphene_matrix_init_identity(&self->window_data->reset_transform);
}

static void _init_overlay(XrdOverlayWindow *self, GxrContext *context) {
	XrdWindow *xrd_window = XRD_WINDOW(self);
	XrdWindowInterface *iface = XRD_WINDOW_GET_IFACE(xrd_window);

	gchar overlay_id_str[25];
	g_sprintf(overlay_id_str, "xrd-window-%d", iface->windows_created);
	self->overlay = gxr_overlay_new(context, overlay_id_str);

	/* g_print ("Created overlay %s\n", overlay_id_str); */

	if (!self->overlay) {
		g_printerr("Overlay unavailable.\n");
		return;
	}

	gxr_overlay_show(self->overlay);

	iface->windows_created++;

	g_signal_connect(xrd_window, "notify::scale",
	                 (GCallback)_update_dimensions_cb, NULL);

	g_signal_connect(xrd_window, "notify::initial-width-meters",
	                 (GCallback)_update_dimensions_cb, NULL);
}

/** xrd_overlay_window_new:
 * Create a new XrdWindow. Note that the window will only have dimensions after
 * a texture is uploaded. */
static XrdOverlayWindow *_new(GxrContext *context, const gchar *title) {
	XrdOverlayWindow *self = (XrdOverlayWindow *)g_object_new(
	    XRD_TYPE_OVERLAY_WINDOW, "title", title, NULL);

	_init_overlay(self, context);

	return self;
}

XrdOverlayWindow *
xrd_overlay_window_new_from_meters(GxrContext *context, const gchar *title,
                                   float width, float height, float ppm) {
	XrdOverlayWindow *self = _new(context, title);

	g_object_set(self, "texture-width", (uint32_t)(width * ppm), "texture-height",
	             (uint32_t)(height * ppm), "initial-width-meters", (double)width,
	             "initial-height-meters", (double)height, NULL);
	return self;
}

XrdOverlayWindow *
xrd_overlay_window_new_from_data(GxrContext *context, XrdWindowData *data) {
	XrdOverlayWindow *self =
	    (XrdOverlayWindow *)g_object_new(XRD_TYPE_OVERLAY_WINDOW, NULL);

	// TODO: avoid unnecessary allocation
	g_free(self->window_data);

	_init_overlay(self, context);

	self->window_data = data;

	_set_transformation(XRD_WINDOW(self), &data->transform);

	return self;
}

XrdOverlayWindow *
xrd_overlay_window_new_from_pixels(GxrContext *context, const gchar *title,
                                   uint32_t width, uint32_t height, float ppm) {
	XrdOverlayWindow *self = _new(context, title);
	g_object_set(self, "texture-width", width, "texture-height", height,
	             "initial-width-meters", (double)width / (double)ppm,
	             "initial-height-meters", (double)height / (double)ppm, NULL);
	return self;
}

XrdOverlayWindow *
xrd_overlay_window_new_from_native(GxrContext *context, const gchar *title,
                                   gpointer native, uint32_t width_pixels,
                                   uint32_t height_pixels, float ppm) {
	XrdOverlayWindow *self = xrd_overlay_window_new_from_pixels(
	    context, title, width_pixels, height_pixels, ppm);
	g_object_set(self, "native", native, NULL);
	return self;
}

static void _set_color(XrdWindow *window, const graphene_vec3_t *color) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	gxr_overlay_set_color(self->overlay, color);
}

static void xrd_overlay_window_constructed(GObject *gobject) {
	G_OBJECT_CLASS(xrd_overlay_window_parent_class)->constructed(gobject);
}

static void xrd_overlay_window_finalize(GObject *gobject) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(gobject);
	if (self->window_data->texture)
		g_clear_object(&self->window_data->texture);
	g_object_unref(self->overlay);

	if (self->window_data->owned_by_window) {
		if (self->window_data->title)
			g_string_free(self->window_data->title, TRUE);
		g_free(self->window_data);
	}

	G_OBJECT_CLASS(xrd_overlay_window_parent_class)->finalize(gobject);
}

static XrdWindowData *_get_data(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	return self->window_data;
}

static void _hide(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	gxr_overlay_hide(self->overlay);
}

static void _show(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	gxr_overlay_show(self->overlay);
}

static void _set_flip_y(XrdWindow *window, gboolean flip_y) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	gxr_overlay_set_flip_y(self->overlay, flip_y);
}

static gboolean _is_visible(XrdWindow *window) {
	XrdOverlayWindow *self = XRD_OVERLAY_WINDOW(window);
	return gxr_overlay_is_visible(self->overlay);
}

static void xrd_overlay_window_window_interface_init(XrdWindowInterface *iface) {
	iface->set_transformation = _set_transformation;
	iface->get_transformation = _get_transformation;
	iface->get_transformation_no_scale = _get_transformation_no_scale;
	iface->submit_texture = _submit_texture;
	iface->set_and_submit_texture = _set_and_submit_texture;
	iface->get_texture = _get_texture;
	iface->poll_event = _poll_event;
	iface->add_child = _add_child;
	iface->set_color = _set_color;
	iface->set_flip_y = _set_flip_y;
	iface->show = _show;
	iface->hide = _hide;
	iface->is_visible = _is_visible;
	iface->get_data = _get_data;
}
