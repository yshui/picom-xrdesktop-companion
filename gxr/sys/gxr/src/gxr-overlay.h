/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OVERLAY_H_
#define GXR_OVERLAY_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <stdint.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <gdk/gdk.h>

#include <graphene.h>
#include <gulkan.h>

#include "gxr-types.h"

G_BEGIN_DECLS

typedef struct _GxrContext GxrContext;

#define GXR_TYPE_OVERLAY gxr_overlay_get_type()
G_DECLARE_DERIVABLE_TYPE (GxrOverlay, gxr_overlay, GXR, OVERLAY, GObject)

struct _GxrOverlayClass
{
  GObjectClass parent;

  void
  (*poll_event) (GxrOverlay *self);

  gboolean
  (*set_mouse_scale) (GxrOverlay *self, float width, float height);

  gboolean
  (*is_visible) (GxrOverlay *self);

  gboolean
  (*thumbnail_is_visible) (GxrOverlay *self);

  gboolean
  (*show) (GxrOverlay *self);

  gboolean
  (*hide) (GxrOverlay *self);

  gboolean
  (*set_sort_order) (GxrOverlay *self, uint32_t sort_order);

  gboolean
  (*clear_texture) (GxrOverlay *self);

  gboolean
  (*get_color) (GxrOverlay *self, graphene_vec3_t *color);

  gboolean
  (*set_color) (GxrOverlay *self, const graphene_vec3_t *color);

  gboolean
  (*set_alpha) (GxrOverlay *self, float alpha);

  gboolean
  (*set_width_meters) (GxrOverlay *self, float meters);

  gboolean
  (*set_transform_absolute) (GxrOverlay *self,
                             graphene_matrix_t *mat);

  gboolean
  (*set_raw) (GxrOverlay *self, guchar *pixels,
              uint32_t width, uint32_t height, uint32_t depth);

  gboolean
  (*get_size_pixels) (GxrOverlay *self, VkExtent2D *size);

  gboolean
  (*get_width_meters) (GxrOverlay *self, float *width);

  gboolean
  (*enable_mouse_input) (GxrOverlay *self);

  gboolean
  (*get_transform_absolute) (GxrOverlay *self,
                             graphene_matrix_t *mat);

  gboolean
  (*show_keyboard) (GxrOverlay *self);

  void
  (*set_keyboard_position) (GxrOverlay   *self,
                            graphene_vec2_t *top_left,
                            graphene_vec2_t *bottom_right);

  gboolean
  (*submit_texture) (GxrOverlay *self,
                     GulkanClient  *client,
                     GulkanTexture *texture);

  gboolean
  (*print_info) (GxrOverlay *self);

  void
  (*set_flip_y) (GxrOverlay *self,
                 gboolean flip_y);
};

GxrOverlay *
gxr_overlay_new (GxrContext *context, gchar* key);

GxrOverlay *
gxr_overlay_new_width (GxrContext *context,
                       gchar      *key,
                       float       meters);

gboolean
gxr_overlay_set_visibility (GxrOverlay *self, gboolean visibility);

gboolean
gxr_overlay_set_cairo_surface_raw (GxrOverlay      *self,
                                   cairo_surface_t *surface);

gboolean
gxr_overlay_set_gdk_pixbuf_raw (GxrOverlay *self, GdkPixbuf * pixbuf);

gboolean
gxr_overlay_get_size_meters (GxrOverlay *self, graphene_vec2_t *size);

gboolean
gxr_overlay_set_translation (GxrOverlay         *self,
                             graphene_point3d_t *translation);

gboolean
gxr_overlay_get_flip_y (GxrOverlay *self);

void
gxr_overlay_set_flip_y (GxrOverlay *self,
                        gboolean flip_y);

/* Virtual methods */
void
gxr_overlay_poll_event (GxrOverlay *self);

gboolean
gxr_overlay_set_mouse_scale (GxrOverlay *self, float width, float height);

gboolean
gxr_overlay_is_visible (GxrOverlay *self);

gboolean
gxr_overlay_thumbnail_is_visible (GxrOverlay *self);

gboolean
gxr_overlay_show (GxrOverlay *self);

gboolean
gxr_overlay_hide (GxrOverlay *self);

gboolean
gxr_overlay_set_sort_order (GxrOverlay *self, uint32_t sort_order);

gboolean
gxr_overlay_clear_texture (GxrOverlay *self);

gboolean
gxr_overlay_get_color (GxrOverlay *self, graphene_vec3_t *color);

gboolean
gxr_overlay_set_color (GxrOverlay *self, const graphene_vec3_t *color);

gboolean
gxr_overlay_set_alpha (GxrOverlay *self, float alpha);

gboolean
gxr_overlay_set_width_meters (GxrOverlay *self, float meters);

gboolean
gxr_overlay_set_transform_absolute (GxrOverlay *self,
                                    graphene_matrix_t *mat);

gboolean
gxr_overlay_set_raw (GxrOverlay *self, guchar *pixels,
                     uint32_t width, uint32_t height, uint32_t depth);

gboolean
gxr_overlay_get_size_pixels (GxrOverlay *self, VkExtent2D *size);

gboolean
gxr_overlay_get_width_meters (GxrOverlay *self, float *width);

gboolean
gxr_overlay_enable_mouse_input (GxrOverlay *self);

gboolean
gxr_overlay_get_transform_absolute (GxrOverlay *self,
                                    graphene_matrix_t *mat);

gboolean
gxr_overlay_show_keyboard (GxrOverlay *self);

void
gxr_overlay_set_keyboard_position (GxrOverlay   *self,
                                   graphene_vec2_t *top_left,
                                   graphene_vec2_t *bottom_right);

gboolean
gxr_overlay_submit_texture (GxrOverlay    *self,
                            GulkanTexture *texture);

gboolean
gxr_overlay_print_info (GxrOverlay *self);

GxrContext *
gxr_overlay_get_context (GxrOverlay *self);

G_END_DECLS

#endif /* GXR_OVERLAY_H_ */
