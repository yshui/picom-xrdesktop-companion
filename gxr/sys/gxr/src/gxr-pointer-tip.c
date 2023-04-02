/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "gxr-pointer-tip.h"

#include <gdk/gdk.h>

#include <gxr.h>

G_DEFINE_INTERFACE (GxrPointerTip, gxr_pointer_tip, G_TYPE_OBJECT)

/* TODO */
static void
gxr_math_matrix_set_translation_point (graphene_matrix_t  *matrix,
                                       graphene_point3d_t *point)
{
  float m[16];
  graphene_matrix_to_float (matrix, m);

  m[12] = point->x;
  m[13] = point->y;
  m[14] = point->z;
  graphene_matrix_init_from_float (matrix, m);
}
static void
graphene_ext_matrix_get_translation_point3d (const graphene_matrix_t *m,
                                             graphene_point3d_t      *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  graphene_point3d_init (res, f[12], f[13], f[14]);
}
/* TODO */


static void
gxr_pointer_tip_default_init (GxrPointerTipInterface *iface)
{
  (void) iface;
}

void
gxr_pointer_tip_update (GxrPointerTip      *self,
                        GxrContext         *context,
                        graphene_matrix_t  *pose,
                        graphene_point3d_t *intersection_point)
{
  graphene_matrix_t transform;
  graphene_matrix_init_from_matrix (&transform, pose);
  gxr_math_matrix_set_translation_point (&transform, intersection_point);
  gxr_pointer_tip_set_transformation (self, &transform);

  gxr_pointer_tip_update_apparent_size (self, context);
}

void
gxr_pointer_tip_set_transformation (GxrPointerTip     *self,
                                    graphene_matrix_t *matrix)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->set_transformation (self, matrix);
}

void
gxr_pointer_tip_get_transformation (GxrPointerTip     *self,
                                    graphene_matrix_t *matrix)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->get_transformation (self, matrix);
}

void
gxr_pointer_tip_show (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->show (self);
}

void
gxr_pointer_tip_hide (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->hide (self);
}

gboolean
gxr_pointer_tip_is_visible (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  return iface->is_visible (self);
}

void
gxr_pointer_tip_set_width_meters (GxrPointerTip *self,
                                  float          meters)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->set_width_meters (self, meters);
}

void
gxr_pointer_tip_submit_texture (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->submit_texture (self);
}

void
gxr_pointer_tip_set_and_submit_texture (GxrPointerTip *self,
                                        GulkanTexture *texture)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  iface->set_and_submit_texture (self, texture);
}

GulkanTexture *
gxr_pointer_tip_get_texture (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  return iface->get_texture (self);
}

GxrPointerTipData*
gxr_pointer_tip_get_data (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  return iface->get_data (self);
}

GulkanClient*
gxr_pointer_tip_get_gulkan_client (GxrPointerTip *self)
{
  GxrPointerTipInterface* iface = GXR_POINTER_TIP_GET_IFACE (self);
  return iface->get_gulkan_client (self);
}

static void
_update_texture (GxrPointerTip *self);

static gboolean
_cancel_animation (GxrPointerTipData *data)
{
  if (data->animation != NULL)
    {
      g_source_remove (data->animation->callback_id);
      g_free (data->animation);
      data->animation = NULL;
      return TRUE;
    }
  else
    return FALSE;
}

static void
_init_texture (GxrPointerTip *self)
{
  GulkanClient *client = gxr_pointer_tip_get_gulkan_client (self);
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);

  GdkPixbuf* pixbuf = gxr_pointer_tip_render (GXR_POINTER_TIP (self), 1.0f);

  GulkanTexture *texture =
    gulkan_texture_new_from_pixbuf (client, pixbuf,
                                    VK_FORMAT_R8G8B8A8_SRGB,
                                    data->upload_layout,
                                    false);
  g_object_unref (pixbuf);

  gxr_pointer_tip_set_and_submit_texture (self, texture);
}

void
gxr_pointer_tip_init_settings (GxrPointerTip     *self,
                               GxrPointerTipData *data)
{
  data->tip = self;

  g_debug ("Initializing pointer tip!");

  GxrPointerTipSettings *s = &data->settings;
  s->keep_apparent_size = TRUE;
  s->width_meters = 0.05f * GXR_TIP_VIEWPORT_SCALE;
  gxr_pointer_tip_set_width_meters (data->tip, s->width_meters);
  graphene_point3d_init (&s->active_color, 0.078f, 0.471f, 0.675f);
  graphene_point3d_init (&s->passive_color, 1.0f, 1.0f, 1.0f);
  s->texture_width = 64;
  s->texture_height = 64;
  s->pulse_alpha = 0.25;
  _init_texture (self);
  _update_texture (self);
}

/** draws a circle in the center of a cairo surface of dimensions WIDTHxHEIGHT.
 * scale affects the radius of the circle and should be in [0,2].
 * a_in is the alpha value at the center, a_out at the outer border. */
static void
_draw_gradient_circle (cairo_t              *cr,
                       int                   w,
                       int                   h,
                       double                radius,
                       graphene_point3d_t   *color,
                       double                a_in,
                       double                a_out)
{
  double center_x = w / 2;
  double center_y = h / 2;

  cairo_pattern_t *pat = cairo_pattern_create_radial (center_x, center_y,
                                                      0.75 * radius,
                                                      center_x, center_y,
                                                      radius);
  cairo_pattern_add_color_stop_rgba (pat, 0,
                                     (double) color->x,
                                     (double) color->y,
                                     (double) color->z, a_in);

  cairo_pattern_add_color_stop_rgba (pat, 1,
                                     (double) color->x,
                                     (double) color->y,
                                     (double) color->z, a_out);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

static GdkPixbuf*
_render_cairo (int                 w,
               int                 h,
               double              radius,
               graphene_point3d_t *color,
               double              pulse_alpha,
               float               progress)
{
  cairo_surface_t *surface =
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);

  cairo_t *cr = cairo_create (surface);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  /* Draw pulse */
  if (progress != 1.0f)
    {
      float pulse_scale = GXR_TIP_VIEWPORT_SCALE * (1.0f - progress);
      graphene_point3d_t white = { 1.0f, 1.0f, 1.0f };
      _draw_gradient_circle (cr, w, h, radius * (double) pulse_scale, &white,
                             pulse_alpha, 0.0);
    }

  cairo_set_operator (cr, CAIRO_OPERATOR_MULTIPLY);

  /* Draw tip */
  _draw_gradient_circle (cr, w, h, radius, color, 1.0, 0.0);

  cairo_destroy (cr);

  /* Since we cannot set a different format for raw upload,
   * we need to use GdkPixbuf to suit OpenVRs needs */
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, w, h);

  cairo_surface_destroy (surface);

  return pixbuf;
}

/** _render:
 * Renders the pointer tip with the desired colors.
 * If background scale is > 1, a transparent white background circle is rendered
 * behind the pointer tip. */
GdkPixbuf*
gxr_pointer_tip_render (GxrPointerTip *self,
                        float          progress)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);

  int w = data->settings.texture_width * GXR_TIP_VIEWPORT_SCALE;
  int h = data->settings.texture_height * GXR_TIP_VIEWPORT_SCALE;

  graphene_point3d_t *color = data->active ? &data->settings.active_color :
                                             &data->settings.passive_color;

  double radius = data->settings.texture_width / 2.0;

  GdkPixbuf *pixbuf =
    _render_cairo (w, h, radius, color, data->settings.pulse_alpha, progress);

  return pixbuf;
}

static gboolean
_animate_cb (gpointer _animation)
{
  GxrPointerTipAnimation *animation = (GxrPointerTipAnimation *) _animation;
  GxrPointerTip *tip = animation->tip;

  GxrPointerTipData *data = gxr_pointer_tip_get_data (tip);

  GulkanTexture *texture = gxr_pointer_tip_get_texture (tip);

  GdkPixbuf* pixbuf = gxr_pointer_tip_render (tip, animation->progress);

  gulkan_texture_upload_pixbuf (texture, pixbuf, data->upload_layout);
  gxr_pointer_tip_submit_texture (tip);

  g_object_unref (pixbuf);

  animation->progress += 0.05f;

  if (animation->progress > 1)
    {
      data->animation = NULL;
      g_free (animation);
      return FALSE;
    }

  return TRUE;
}

void
gxr_pointer_tip_animate_pulse (GxrPointerTip *self)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);
  if (data->animation != NULL)
    gxr_pointer_tip_set_active (data->tip, data->active);

  data->animation = g_malloc (sizeof (GxrPointerTipAnimation));
  data->animation->progress = 0;
  data->animation->tip = data->tip;
  data->animation->callback_id = g_timeout_add (20, _animate_cb,
                                                data->animation);
}

static void
_update_texture (GxrPointerTip *self)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);

  GdkPixbuf* pixbuf = gxr_pointer_tip_render (self, 1.0f);
  GulkanTexture *texture = gxr_pointer_tip_get_texture (self);

  gulkan_texture_upload_pixbuf (texture, pixbuf, data->upload_layout);
  g_object_unref (pixbuf);

  gxr_pointer_tip_submit_texture (self);
}

/** gxr_pointer_tip_set_active:
 * Changes whether the active or inactive style is rendered.
 * Also cancels animations. */
void
gxr_pointer_tip_set_active (GxrPointerTip *self,
                            gboolean       active)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);

  if (gxr_pointer_tip_get_texture (self) == NULL)
    return;

  /* New texture needs to be rendered when
   *  - animation is being cancelled
   *  - active status changes
   * Otherwise the texture should already show the current active status. */

  gboolean animation_cancelled = _cancel_animation (data);
  if (!animation_cancelled && data->active == active)
    return;

  data->active = active;

  _update_texture (self);
}

/* note: Move pointer tip to the desired location before calling. */
void
gxr_pointer_tip_update_apparent_size (GxrPointerTip *self,
                                      GxrContext    *context)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);

  if (!data->settings.keep_apparent_size)
    return;

  graphene_matrix_t tip_pose;
  gxr_pointer_tip_get_transformation (self, &tip_pose);

  graphene_point3d_t tip_point;
  graphene_ext_matrix_get_translation_point3d (&tip_pose, &tip_point);

  graphene_matrix_t hmd_pose;
  gboolean has_pose = gxr_context_get_head_pose (context, &hmd_pose);
  if (!has_pose)
    {
      gxr_pointer_tip_set_width_meters (self, data->settings.width_meters);
      return;
    }

  graphene_point3d_t hmd_point;
  graphene_ext_matrix_get_translation_point3d (&hmd_pose, &hmd_point);

  float distance = graphene_point3d_distance (&tip_point, &hmd_point, NULL);

  float w = data->settings.width_meters
            / GXR_TIP_APPARENT_SIZE_DISTANCE * distance;

  gxr_pointer_tip_set_width_meters (self, w);
}

void
gxr_pointer_tip_update_texture_resolution (GxrPointerTip *self,
                                           int            width,
                                           int            height)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);
  GxrPointerTipSettings *s = &data->settings;
  s->texture_width = width;
  s->texture_height = height;

  _init_texture (data->tip);
}

void
gxr_pointer_tip_update_color (GxrPointerTip      *self,
                              gboolean            active_color,
                              graphene_point3d_t *color)

{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);
  GxrPointerTipSettings *s = &data->settings;

  if (active_color)
    graphene_point3d_init_from_point (&s->active_color, color);
  else
    graphene_point3d_init_from_point (&s->passive_color, color);

  if ((!data->active && !active_color) || (data->active && active_color))
    {
      _cancel_animation (data);
      _update_texture (data->tip);
    }
}

void
gxr_pointer_tip_update_pulse_alpha (GxrPointerTip *self,
                                    double         alpha)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);
  GxrPointerTipSettings *s = &data->settings;
  s->pulse_alpha = alpha;
}

void
gxr_pointer_tip_update_keep_apparent_size (GxrPointerTip *self,
                                           gboolean       keep_apparent_size)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);
  GxrPointerTipSettings *s = &data->settings;

  s->keep_apparent_size = keep_apparent_size;
  gxr_pointer_tip_set_width_meters (data->tip, s->width_meters);
}

void
gxr_pointer_tip_update_width_meters (GxrPointerTip *self,
                                     float          width)
{
  GxrPointerTipData *data = gxr_pointer_tip_get_data (self);
  GxrPointerTipSettings *s = &data->settings;

  s->width_meters = width * GXR_TIP_VIEWPORT_SCALE;
  gxr_pointer_tip_set_width_meters (data->tip, s->width_meters);
}
