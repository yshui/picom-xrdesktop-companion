/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib/gprintf.h>

#include <gdk/gdk.h>

#include "gxr-overlay-private.h"
#include "gxr-time.h"
#include "gxr-config.h"

#include "gxr-context-private.h"

typedef struct _GxrOverlayPrivate
{
  GObjectClass  parent_class;
  GxrContext   *context;
  gboolean      flip_y;
} GxrOverlayPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrOverlay, gxr_overlay, G_TYPE_OBJECT)

enum {
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SHOW,
  HIDE,
  DESTROY,
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  LAST_SIGNAL
};

static guint overlay_signals[LAST_SIGNAL] = { 0 };

static void
_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gxr_overlay_parent_class)->finalize (gobject);
}

static void
gxr_overlay_class_init (GxrOverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
  overlay_signals[MOTION_NOTIFY_EVENT] =
    g_signal_new ("motion-notify-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[SHOW] =
    g_signal_new ("show",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_FIRST,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  overlay_signals[HIDE] =
    g_signal_new ("hide",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_FIRST,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  overlay_signals[DESTROY] =
    g_signal_new ("destroy",
                   G_TYPE_FROM_CLASS (klass),
                  (GSignalFlags)
                     (G_SIGNAL_RUN_CLEANUP |
                      G_SIGNAL_NO_RECURSE |
                      G_SIGNAL_NO_HOOKS),
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  overlay_signals[KEYBOARD_PRESS_EVENT] =
    g_signal_new ("keyboard-press-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  overlay_signals[KEYBOARD_CLOSE_EVENT] =
    g_signal_new ("keyboard-close-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
gxr_overlay_init (GxrOverlay *self)
{
  GxrOverlayPrivate *priv = gxr_overlay_get_instance_private (self);
  priv->flip_y = FALSE;
  priv->context = NULL;
}

GxrOverlay *
gxr_overlay_new (GxrContext *context, gchar* key)
{
  GxrOverlay *self = gxr_context_new_overlay (context, key);
  GxrOverlayPrivate *priv = gxr_overlay_get_instance_private (self);
  priv->context = context;
  return self;
}

GxrOverlay *
gxr_overlay_new_width (GxrContext *context,
                       gchar      *key,
                       float       meters)
{
  GxrOverlay *self = gxr_overlay_new (context, key);
  if (!gxr_overlay_set_width_meters (self, meters))
    return NULL;

  return self;
}

gboolean
gxr_overlay_set_visibility (GxrOverlay *self, gboolean visibility)
{
  if (visibility)
    return gxr_overlay_show (self);
  else
    return gxr_overlay_hide (self);
}

gboolean
gxr_overlay_set_gdk_pixbuf_raw (GxrOverlay *self, GdkPixbuf * pixbuf)
{
  guint width = (guint)gdk_pixbuf_get_width (pixbuf);
  guint height = (guint)gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  uint32_t depth = 3;
  switch (gdk_pixbuf_get_n_channels (pixbuf))
  {
    case 3:
      depth = 3;
      break;
    case 4:
      depth = 4;
      break;
    default:
      depth = 3;
  }

  return gxr_overlay_set_raw (self, pixels, width, height, depth);
}

gboolean
gxr_overlay_set_cairo_surface_raw (GxrOverlay   *self,
                                   cairo_surface_t *surface)
{
  guchar *pixels = cairo_image_surface_get_data (surface);

  guint width = (guint)cairo_image_surface_get_width (surface);
  guint height = (guint)cairo_image_surface_get_height (surface);

  uint32_t depth;
  cairo_format_t cr_format = cairo_image_surface_get_format (surface);
  switch (cr_format)
    {
    case CAIRO_FORMAT_ARGB32:
      depth = 4;
      break;
    case CAIRO_FORMAT_RGB24:
      depth = 3;
      break;
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB16_565:
    case CAIRO_FORMAT_RGB30:
      g_printerr ("Unsupported Cairo format\n");
      return FALSE;
    case CAIRO_FORMAT_INVALID:
      g_printerr ("Invalid Cairo format\n");
      return FALSE;
    default:
      g_printerr ("Unknown Cairo format\n");
      return FALSE;
    }

  return gxr_overlay_set_raw (self, pixels, width, height, depth);
}

/*
 * There is no function to get the height or aspect ratio of an overlay,
 * so we need to calculate it from width + texture size
 * the texture aspect ratio should be preserved.
 */

gboolean
gxr_overlay_get_size_meters (GxrOverlay *self, graphene_vec2_t *size)
{
  gfloat width_meters;
  if (!gxr_overlay_get_width_meters (self, &width_meters))
    return FALSE;

  VkExtent2D size_pixels;
  if (!gxr_overlay_get_size_pixels (self, &size_pixels))
    return FALSE;

  gfloat aspect = (gfloat) size_pixels.width / (gfloat) size_pixels.height;
  gfloat height_meters = width_meters / aspect;

  graphene_vec2_init (size, width_meters, height_meters);

  return TRUE;
}

gboolean
gxr_overlay_set_translation (GxrOverlay      *self,
                             graphene_point3d_t *translation)
{
  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, translation);
  return gxr_overlay_set_transform_absolute (self, &transform);
}

gboolean
gxr_overlay_get_flip_y (GxrOverlay *self)
{
  GxrOverlayPrivate *priv = gxr_overlay_get_instance_private (self);
  return priv->flip_y;
}

void
gxr_overlay_set_flip_y (GxrOverlay *self,
                        gboolean flip_y)
{
  GxrOverlayPrivate *priv = gxr_overlay_get_instance_private (self);
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_flip_y == NULL)
    return;

  klass->set_flip_y (self, flip_y);
  priv->flip_y = flip_y;
}

void
gxr_overlay_emit_motion_notify (GxrOverlay *self, GdkEvent *event)
{
  g_signal_emit (self, overlay_signals[MOTION_NOTIFY_EVENT], 0, event);
}

void
gxr_overlay_emit_button_press (GxrOverlay *self, GdkEvent *event)
{
  g_signal_emit (self, overlay_signals[BUTTON_PRESS_EVENT], 0, event);
}

void
gxr_overlay_emit_button_release (GxrOverlay *self, GdkEvent *event)
{
  g_signal_emit (self, overlay_signals[BUTTON_RELEASE_EVENT], 0, event);
}

void
gxr_overlay_emit_show (GxrOverlay *self)
{
  g_signal_emit (self, overlay_signals[SHOW], 0);
}

void
gxr_overlay_emit_hide (GxrOverlay *self)
{
  g_signal_emit (self, overlay_signals[HIDE], 0);
}

void
gxr_overlay_emit_destroy (GxrOverlay *self)
{
  g_signal_emit (self, overlay_signals[DESTROY], 0);
}

void
gxr_overlay_emit_keyboard_press (GxrOverlay *self, GdkEvent *event)
{
  g_signal_emit (self, overlay_signals[KEYBOARD_PRESS_EVENT], 0, event);
}

void
gxr_overlay_emit_keyboard_close (GxrOverlay *self)
{
  g_signal_emit (self, overlay_signals[KEYBOARD_CLOSE_EVENT], 0);
}

gboolean
gxr_overlay_show (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->show == NULL)
    return FALSE;
  return klass->show (self);
}

gboolean
gxr_overlay_hide (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->hide == NULL)
    return FALSE;
  return klass->hide (self);
}

gboolean
gxr_overlay_is_visible (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->is_visible == NULL)
    return FALSE;
  return klass->is_visible (self);
}

gboolean
gxr_overlay_thumbnail_is_visible (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->thumbnail_is_visible == NULL)
    return FALSE;
  return klass->thumbnail_is_visible (self);
}

gboolean
gxr_overlay_set_sort_order (GxrOverlay *self, uint32_t sort_order)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_sort_order == NULL)
    return FALSE;
  return klass->set_sort_order (self, sort_order);
}

gboolean
gxr_overlay_enable_mouse_input (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->enable_mouse_input == NULL)
    return FALSE;
  return klass->enable_mouse_input (self);
}

void
gxr_overlay_poll_event (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->poll_event == NULL)
    return;
  klass->poll_event (self);
}

gboolean
gxr_overlay_set_mouse_scale (GxrOverlay *self, float width, float height)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_mouse_scale == NULL)
    return FALSE;
  return klass->set_mouse_scale (self, width, height);
}

gboolean
gxr_overlay_clear_texture (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->clear_texture == NULL)
    return FALSE;
  return klass->clear_texture (self);
}

gboolean
gxr_overlay_get_color (GxrOverlay *self, graphene_vec3_t *color)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->get_color == NULL)
    return FALSE;
  return klass->get_color (self, color);
}

gboolean
gxr_overlay_set_color (GxrOverlay *self, const graphene_vec3_t *color)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_color == NULL)
    return FALSE;
  return klass->set_color (self, color);
}

gboolean
gxr_overlay_set_alpha (GxrOverlay *self, float alpha)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_alpha == NULL)
    return FALSE;
  return klass->set_alpha (self, alpha);
}

gboolean
gxr_overlay_set_width_meters (GxrOverlay *self, float meters)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_width_meters == NULL)
    return FALSE;
  return klass->set_width_meters (self, meters);
}

gboolean
gxr_overlay_set_transform_absolute (GxrOverlay *self,
                                    graphene_matrix_t *mat)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_transform_absolute == NULL)
    return FALSE;
  return klass->set_transform_absolute (self, mat);
}

gboolean
gxr_overlay_get_transform_absolute (GxrOverlay *self,
                                    graphene_matrix_t *mat)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->get_transform_absolute == NULL)
    return FALSE;
  return klass->get_transform_absolute (self, mat);
}

gboolean
gxr_overlay_set_raw (GxrOverlay *self, guchar *pixels,
                     uint32_t width, uint32_t height, uint32_t depth)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_raw == NULL)
    return FALSE;
  return klass->set_raw (self, pixels, width, height, depth);
}

gboolean
gxr_overlay_get_size_pixels (GxrOverlay *self, VkExtent2D *size)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->get_size_pixels == NULL)
    return FALSE;
  return klass->get_size_pixels (self, size);
}

gboolean
gxr_overlay_get_width_meters (GxrOverlay *self, float *width)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->get_width_meters == NULL)
    return FALSE;
  return klass->get_width_meters (self, width);
}

gboolean
gxr_overlay_show_keyboard (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->show_keyboard == NULL)
    return FALSE;
  return klass->show_keyboard (self);
}

void
gxr_overlay_set_keyboard_position (GxrOverlay      *self,
                                   graphene_vec2_t *top_left,
                                   graphene_vec2_t *bottom_right)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->set_keyboard_position == NULL)
    return;
  klass->set_keyboard_position (self, top_left, bottom_right);
}

gboolean
gxr_overlay_submit_texture (GxrOverlay    *self,
                            GulkanTexture *texture)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->submit_texture == NULL)
    return FALSE;

  GxrOverlayPrivate *priv = gxr_overlay_get_instance_private (self);
  GulkanClient *client = gxr_context_get_gulkan (priv->context);
  return klass->submit_texture (self, client, texture);
}

gboolean
gxr_overlay_print_info (GxrOverlay *self)
{
  GxrOverlayClass *klass = GXR_OVERLAY_GET_CLASS (self);
  if (klass->print_info == NULL)
    return FALSE;
  return klass->print_info (self);
}

GxrContext *
gxr_overlay_get_context (GxrOverlay *self)
{
  GxrOverlayPrivate *priv = gxr_overlay_get_instance_private (self);
  return priv->context;
}
