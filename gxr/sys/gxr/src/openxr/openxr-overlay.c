/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib/gprintf.h>
#include <gdk/gdk.h>

#include "gxr-time.h"
#include "gxr-config.h"

#include "openxr-overlay.h"

struct _OpenXROverlay
{
  GxrOverlayClass parent;
};

G_DEFINE_TYPE (OpenXROverlay, openxr_overlay, GXR_TYPE_OVERLAY)

static void
openxr_overlay_init (OpenXROverlay *self)
{
  (void) self;
}

OpenXROverlay *
openxr_overlay_new (void)
{
  g_print ("stub: OpenXR overlays are not implemented.\n");
  return (OpenXROverlay*) g_object_new (OPENXR_TYPE_OVERLAY, 0);
}

static void
_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (openxr_overlay_parent_class)->finalize (gobject);
}

static gboolean
_show (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static gboolean
_hide (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static gboolean
_is_visible (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static gboolean
_thumbnail_is_visible (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static gboolean
_set_sort_order (GxrOverlay *overlay, uint32_t sort_order)
{
  (void) overlay;
  (void) sort_order;
  return TRUE;
}


static gboolean
_enable_mouse_input (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static void
_poll_event (GxrOverlay *overlay)
{
  (void) overlay;
}

static gboolean
_set_mouse_scale (GxrOverlay *overlay, float width, float height)
{
  (void) overlay;
  (void) width;
  (void) height;
  return TRUE;
}

static gboolean
_clear_texture (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static gboolean
_get_color (GxrOverlay *overlay, graphene_vec3_t *color)
{
  (void) overlay;
  (void) color;
  return TRUE;
}

static gboolean
_set_color (GxrOverlay *overlay, const graphene_vec3_t *color)
{
  (void) overlay;
  (void) color;
  return TRUE;
}

static gboolean
_set_alpha (GxrOverlay *overlay, float alpha)
{
  (void) overlay;
  (void) alpha;
  return TRUE;
}

static gboolean
_set_width_meters (GxrOverlay *overlay, float meters)
{
  (void) overlay;
  (void) meters;
  return TRUE;
}

static gboolean
_set_transform_absolute (GxrOverlay *overlay,
                         graphene_matrix_t *mat)
{
  (void) overlay;
  (void) mat;
  return TRUE;
}

static gboolean
_get_transform_absolute (GxrOverlay *overlay,
                         graphene_matrix_t *mat)
{
  (void) overlay;
  (void) mat;
  return TRUE;
}

static gboolean
_set_raw (GxrOverlay *overlay, guchar *pixels,
          uint32_t width, uint32_t height, uint32_t depth)
{
  (void) overlay;
  (void) pixels;
  (void) width;
  (void) height;
  (void) depth;
  return TRUE;
}

static gboolean
_get_size_pixels (GxrOverlay *overlay, VkExtent2D *size)
{
  (void) overlay;
  (void) size;
  return TRUE;
}

static gboolean
_get_width_meters (GxrOverlay *overlay, float *width)
{
  (void) overlay;
  (void) width;
  return TRUE;
}

static gboolean
_show_keyboard (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static void
_set_keyboard_position (GxrOverlay      *overlay,
                        graphene_vec2_t *top_left,
                        graphene_vec2_t *bottom_right)
{
  (void) overlay;
  (void) top_left;
  (void) bottom_right;
}

static void
_set_flip_y (GxrOverlay *overlay,
             gboolean flip_y)
{
  (void) overlay;
  (void) flip_y;
}

/* Submit frame to OpenXR runtime */
static gboolean
_submit_texture (GxrOverlay    *overlay,
                 GulkanClient  *client,
                 GulkanTexture *texture)
{
  (void) overlay;
  (void) client;
  (void) texture;
  return TRUE;
}

static gboolean
_print_info (GxrOverlay *overlay)
{
  (void) overlay;
  return TRUE;
}

static void
openxr_overlay_class_init (OpenXROverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;

  GxrOverlayClass *parent_class = GXR_OVERLAY_CLASS (klass);
  parent_class->poll_event = _poll_event;
  parent_class->set_mouse_scale = _set_mouse_scale;
  parent_class->is_visible = _is_visible;
  parent_class->thumbnail_is_visible = _thumbnail_is_visible;
  parent_class->show = _show;
  parent_class->hide = _hide;
  parent_class->set_sort_order = _set_sort_order;
  parent_class->clear_texture = _clear_texture;
  parent_class->get_color = _get_color;
  parent_class->set_color = _set_color;
  parent_class->set_alpha = _set_alpha;
  parent_class->set_width_meters = _set_width_meters;
  parent_class->set_transform_absolute = _set_transform_absolute;
  parent_class->set_raw = _set_raw;
  parent_class->get_size_pixels = _get_size_pixels;
  parent_class->get_width_meters = _get_width_meters;
  parent_class->enable_mouse_input = _enable_mouse_input;
  parent_class->get_transform_absolute = _get_transform_absolute;
  parent_class->show_keyboard = _show_keyboard;
  parent_class->set_keyboard_position = _set_keyboard_position;
  parent_class->submit_texture = _submit_texture;
  parent_class->print_info = _print_info;
  parent_class->set_flip_y = _set_flip_y;
}
