/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-pointer.h"
#include "graphene-ext.h"

G_DEFINE_INTERFACE (GxrPointer, gxr_pointer, G_TYPE_OBJECT)

static void
gxr_pointer_default_init (GxrPointerInterface *iface)
{
  (void) iface;
}

void
gxr_pointer_move (GxrPointer        *self,
                  graphene_matrix_t *transform)
{
  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  if (iface->move)
    iface->move (self, transform);
}

void
gxr_pointer_set_length (GxrPointer *self,
                        float       length)
{
  GxrPointerData *data = gxr_pointer_get_data (self);
  if (length == data->length)
    return;

  data->length = length;

  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  if (iface->set_length)
    iface->set_length (self, length);
}

float
gxr_pointer_get_default_length (GxrPointer *self)
{
  GxrPointerData *data = gxr_pointer_get_data (self);
  return data->default_length;
}

void
gxr_pointer_reset_length (GxrPointer *self)
{
  GxrPointerData *data = gxr_pointer_get_data (self);
  gxr_pointer_set_length (self, data->default_length);
}

GxrPointerData*
gxr_pointer_get_data (GxrPointer *self)
{
  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  return iface->get_data (self);
}

void
gxr_pointer_init (GxrPointer *self)
{
  GxrPointerData *data = gxr_pointer_get_data (self);
  data->start_offset = -0.02f;
  data->default_length = 5.0f;
  data->length = data->default_length;
  data->visible = TRUE;
}

void
gxr_pointer_set_transformation (GxrPointer        *self,
                                graphene_matrix_t *matrix)
{
  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  iface->set_transformation (self, matrix);
}

void
gxr_pointer_get_transformation (GxrPointer        *self,
                                graphene_matrix_t *matrix)
{
  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  iface->get_transformation (self, matrix);
}

void
gxr_pointer_get_ray (GxrPointer     *self,
                     graphene_ray_t *res)
{
  GxrPointerData *data = gxr_pointer_get_data (self);

  graphene_matrix_t mat;
  gxr_pointer_get_transformation (self, &mat);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, data->start_offset, 1);
  graphene_matrix_transform_vec4 (&mat, &start, &start);

  graphene_vec4_t end;
  graphene_vec4_init (&end, 0, 0, -data->length, 1);
  graphene_matrix_transform_vec4 (&mat, &end, &end);

  graphene_vec4_t direction_vec4;
  graphene_vec4_subtract (&end, &start, &direction_vec4);

  graphene_point3d_t origin;
  graphene_vec3_t direction;

  graphene_vec3_t vec3_start;
  graphene_vec4_get_xyz (&start, &vec3_start);
  graphene_point3d_init_from_vec3 (&origin, &vec3_start);

  graphene_vec4_get_xyz (&direction_vec4, &direction);

  graphene_ray_init (res, &origin, &direction);
}

gboolean
gxr_pointer_get_plane_intersection (GxrPointer        *self,
                                    graphene_plane_t  *plane,
                                    graphene_matrix_t *plane_transform,
                                    float              plane_aspect,
                                    float             *distance,
                                    graphene_vec3_t   *res)
{
  graphene_ray_t ray;
  gxr_pointer_get_ray (self, &ray);

  *distance = graphene_ray_get_distance_to_plane (&ray, plane);
  if (*distance == INFINITY)
    return FALSE;

  graphene_ray_get_direction (&ray, res);
  graphene_vec3_scale (res, *distance, res);

  graphene_vec3_t origin;
  graphene_ext_ray_get_origin_vec3 (&ray, &origin);
  graphene_vec3_add (&origin, res, res);

  graphene_matrix_t inverse_plane_transform;

  graphene_matrix_inverse (plane_transform, &inverse_plane_transform);

  graphene_vec4_t intersection_vec4;
  graphene_vec4_init_from_vec3 (&intersection_vec4, res, 1.0f);

  graphene_vec4_t intersection_origin;
  graphene_matrix_transform_vec4 (&inverse_plane_transform,
                                  &intersection_vec4,
                                  &intersection_origin);

  float f[4];
  graphene_vec4_to_float (&intersection_origin, f);

  if (f[0] >= -plane_aspect / 2.0f && f[0] <= plane_aspect / 2.0f
      && f[1] >= -0.5f && f[1] <= 0.5f)
    return TRUE;

  return FALSE;
}

void
gxr_pointer_show (GxrPointer *self)
{
  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  GxrPointerData *data = gxr_pointer_get_data (self);

  if (!data->render_ray)
    {
      return;
    }

  if (iface->show)
    iface->show (self);

  data->visible = TRUE;
}

void
gxr_pointer_hide (GxrPointer *self)
{
  GxrPointerInterface* iface = GXR_POINTER_GET_IFACE (self);
  if (iface->hide)
    iface->hide (self);

  GxrPointerData *data = gxr_pointer_get_data (self);
  data->visible = FALSE;
}

gboolean
gxr_pointer_is_visible (GxrPointer *self)
{
  GxrPointerData *data = gxr_pointer_get_data (self);

  if (!data->render_ray)
    return FALSE;

  return data->visible;
}

void
gxr_pointer_update_render_ray (GxrPointer *self, gboolean render_ray)
{
  GxrPointerData *data = gxr_pointer_get_data (self);

  data->render_ray = render_ray;

  if (!data->visible && render_ray)
    gxr_pointer_show (self);
  else if (data->visible && !render_ray)
    gxr_pointer_hide (self);
}
