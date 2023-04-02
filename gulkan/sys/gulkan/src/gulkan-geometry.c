/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-geometry.h"

void
gulkan_geometry_append_axes (GulkanVertexBuffer *self,
                             graphene_vec4_t    *center,
                             float               length,
                             graphene_matrix_t  *mat)
{
  graphene_vec4_t center_transformed;
  graphene_matrix_transform_vec4 (mat, center, &center_transformed);

  for (int i = 0; i < 3; ++i)
    {
      float point_float[4] = {0, 0, 0, 1};
      point_float[i] += length;
      graphene_vec4_t point;
      graphene_vec4_init_from_float (&point, point_float);
      graphene_matrix_transform_vec4 (mat, &point, &point);

      /* R, G, B for X, Y, Z */
      float color_float[3] = {0, 0, 0};
      color_float[i] = 1.0;
      graphene_vec3_t color;
      graphene_vec3_init_from_float (&color, color_float);

      gulkan_vertex_buffer_append_with_color (self,
                                              &center_transformed,
                                              &color);

      gulkan_vertex_buffer_append_with_color (self, &point, &color);

    }
}

void
gulkan_geometry_append_ray (GulkanVertexBuffer *self,
                            graphene_vec4_t    *center,
                            float               length,
                            graphene_matrix_t  *mat)
{
  graphene_vec4_t start;
  graphene_matrix_transform_vec4 (mat, center, &start);

  graphene_vec4_t end;
  graphene_vec4_init (&end, 0, 0, -length, 1);
  graphene_matrix_transform_vec4 (mat, &end, &end);

  graphene_vec3_t color;
  graphene_vec3_init (&color, .8f, .8f, .9f);


  gulkan_vertex_buffer_append_with_color (self, &start, &color);
  gulkan_vertex_buffer_append_with_color (self, &end, &color);
}

void
gulkan_geometry_append_plane (GulkanVertexBuffer *self,
                              graphene_point_t   *from,
                              graphene_point_t   *to,
                              graphene_matrix_t  *mat)
{
  graphene_vec4_t a, b, c, d;
  graphene_vec4_init (&a, from->x, from->y, 0, 1);
  graphene_vec4_init (&b, to->x,   from->y, 0, 1);
  graphene_vec4_init (&c, to->x,   to->y,   0, 1);
  graphene_vec4_init (&d, from->x, to->y,   0, 1);

  graphene_matrix_transform_vec4 (mat, &a, &a);
  graphene_matrix_transform_vec4 (mat, &b, &b);
  graphene_matrix_transform_vec4 (mat, &c, &c);
  graphene_matrix_transform_vec4 (mat, &d, &d);

  /* Create triangles */
  gulkan_vertex_buffer_append_position_uv (self, &a, 0, 1);
  gulkan_vertex_buffer_append_position_uv (self, &b, 1, 1);
  gulkan_vertex_buffer_append_position_uv (self, &c, 1, 0);
  gulkan_vertex_buffer_append_position_uv (self, &c, 1, 0);
  gulkan_vertex_buffer_append_position_uv (self, &d, 0, 0);
  gulkan_vertex_buffer_append_position_uv (self, &a, 0, 1);
}
