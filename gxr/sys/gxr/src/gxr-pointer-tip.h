
/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_POINTER_TIP_H_
#define GXR_POINTER_TIP_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>
#include <gulkan.h>

#include "gxr-context.h"

G_BEGIN_DECLS

#define GXR_TYPE_POINTER_TIP gxr_pointer_tip_get_type()
G_DECLARE_INTERFACE (GxrPointerTip, gxr_pointer_tip, GXR, POINTER_TIP, GObject)

/*
 * Since the pulse animation surrounds the tip and would
 * exceed the canvas size, we need to scale it to fit the pulse.
 */
#define GXR_TIP_VIEWPORT_SCALE 3

/*
 * The distance in meters for which apparent size and regular size are equal.
 */
#define GXR_TIP_APPARENT_SIZE_DISTANCE 3.0f

typedef struct {
  GxrPointerTip *tip;
  float progress;
  guint callback_id;
} GxrPointerTipAnimation;

typedef struct {
  gboolean keep_apparent_size;
  float width_meters;

  graphene_point3d_t active_color;
  graphene_point3d_t passive_color;

  double pulse_alpha;

  int texture_width;
  int texture_height;
} GxrPointerTipSettings;

typedef struct {
  GxrPointerTip *tip;

  gboolean active;

  VkImageLayout upload_layout;

  GxrPointerTipSettings settings;

  /* Pointer to the data of the currently running animation.
   * Must be freed when an animation callback is cancelled. */
  GxrPointerTipAnimation *animation;
} GxrPointerTipData;

struct _GxrPointerTipInterface
{
  GTypeInterface parent;

  void
  (*set_transformation) (GxrPointerTip     *self,
                         graphene_matrix_t *matrix);

  void
  (*get_transformation) (GxrPointerTip     *self,
                         graphene_matrix_t *matrix);

  void
  (*show) (GxrPointerTip *self);

  void
  (*hide) (GxrPointerTip *self);

  gboolean
  (*is_visible) (GxrPointerTip *self);

  void
  (*set_width_meters) (GxrPointerTip *self,
                       float          meters);

  void
  (*submit_texture) (GxrPointerTip *self);

  GulkanTexture *
  (*get_texture) (GxrPointerTip *self);

  void
  (*set_and_submit_texture) (GxrPointerTip *self,
                             GulkanTexture *texture);

  GxrPointerTipData*
  (*get_data) (GxrPointerTip *self);

  GulkanClient*
  (*get_gulkan_client) (GxrPointerTip *self);
};

void
gxr_pointer_tip_update_apparent_size (GxrPointerTip *self,
                                      GxrContext    *context);

void
gxr_pointer_tip_update (GxrPointerTip      *self,
                        GxrContext         *context,
                        graphene_matrix_t  *pose,
                        graphene_point3d_t *intersection_point);

void
gxr_pointer_tip_set_active (GxrPointerTip *self,
                            gboolean       active);

void
gxr_pointer_tip_animate_pulse (GxrPointerTip *self);

void
gxr_pointer_tip_set_transformation (GxrPointerTip     *self,
                                    graphene_matrix_t *matrix);

void
gxr_pointer_tip_get_transformation (GxrPointerTip     *self,
                                    graphene_matrix_t *matrix);

void
gxr_pointer_tip_show (GxrPointerTip *self);

void
gxr_pointer_tip_hide (GxrPointerTip *self);

gboolean
gxr_pointer_tip_is_visible (GxrPointerTip *self);

void
gxr_pointer_tip_set_width_meters (GxrPointerTip *self,
                                  float          meters);

void
gxr_pointer_tip_submit_texture (GxrPointerTip *self);

void
gxr_pointer_tip_set_and_submit_texture (GxrPointerTip *self,
                                        GulkanTexture *texture);

GulkanTexture *
gxr_pointer_tip_get_texture (GxrPointerTip *self);

void
gxr_pointer_tip_init_settings (GxrPointerTip     *self,
                               GxrPointerTipData *data);

GdkPixbuf*
gxr_pointer_tip_render (GxrPointerTip *self,
                        float          progress);

GxrPointerTipData*
gxr_pointer_tip_get_data (GxrPointerTip *self);

GulkanClient*
gxr_pointer_tip_get_gulkan_client (GxrPointerTip *self);


void
gxr_pointer_tip_update_texture_resolution (GxrPointerTip *self,
                                           int            width,
                                           int            height);

void
gxr_pointer_tip_update_color (GxrPointerTip      *self,
                              gboolean            active_color,
                              graphene_point3d_t *color);

void
gxr_pointer_tip_update_pulse_alpha (GxrPointerTip *self,
                                    double         alpha);

void
gxr_pointer_tip_update_keep_apparent_size (GxrPointerTip *self,
                                           gboolean       keep_apparent_size);
void
gxr_pointer_tip_update_width_meters (GxrPointerTip *self,
                                     float          width);

G_END_DECLS

#endif /* GXR_POINTER_TIP_H_ */
