/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_POINTER_H_
#define GXR_POINTER_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

G_BEGIN_DECLS

#define GXR_TYPE_POINTER gxr_pointer_get_type()
G_DECLARE_INTERFACE (GxrPointer, gxr_pointer, GXR, POINTER, GObject)

typedef struct {
  float start_offset;
  float length;
  float default_length;
  gboolean visible;
  gboolean render_ray;
} GxrPointerData;

struct _GxrPointerInterface
{
  GTypeInterface parent;

  void
  (*move) (GxrPointer        *self,
           graphene_matrix_t *transform);

  void
  (*set_length) (GxrPointer *self,
                 float       length);

  GxrPointerData*
  (*get_data) (GxrPointer *self);

  void
  (*set_transformation) (GxrPointer        *self,
                         graphene_matrix_t *matrix);

  void
  (*get_transformation) (GxrPointer        *self,
                         graphene_matrix_t *matrix);

  void
  (*set_selected_object) (GxrPointer *pointer,
                          gpointer    object);

  void
  (*show) (GxrPointer *self);

  void
  (*hide) (GxrPointer *self);
};

void
gxr_pointer_move (GxrPointer *self,
                  graphene_matrix_t *transform);

void
gxr_pointer_set_length (GxrPointer *self,
                        float       length);

float
gxr_pointer_get_default_length (GxrPointer *self);

void
gxr_pointer_reset_length (GxrPointer *self);

GxrPointerData*
gxr_pointer_get_data (GxrPointer *self);

void
gxr_pointer_init (GxrPointer *self);

void
gxr_pointer_set_transformation (GxrPointer        *self,
                                graphene_matrix_t *matrix);

void
gxr_pointer_get_transformation (GxrPointer        *self,
                                graphene_matrix_t *matrix);

void
gxr_pointer_get_ray (GxrPointer     *self,
                     graphene_ray_t *res);

gboolean
gxr_pointer_get_plane_intersection (GxrPointer        *self,
                                    graphene_plane_t  *plane,
                                    graphene_matrix_t *plane_transform,
                                    float              plane_aspect,
                                    float             *distance,
                                    graphene_vec3_t   *res);

void
gxr_pointer_show (GxrPointer *self);

void
gxr_pointer_hide (GxrPointer *self);

gboolean
gxr_pointer_is_visible (GxrPointer *self);

void
gxr_pointer_update_render_ray (GxrPointer *self, gboolean render_ray);

G_END_DECLS

#endif /* GXR_POINTER_H_ */
