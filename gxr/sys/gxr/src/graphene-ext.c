/*
 * Graphene Extensions
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include "graphene-ext.h"
#include <inttypes.h>

// TODO: Move to upstream

void
graphene_ext_quaternion_to_float (const graphene_quaternion_t *q,
                                  float                       *dest)
{
  graphene_vec4_t v;
  graphene_quaternion_to_vec4 (q, &v);
  graphene_vec4_to_float (&v, dest);
}

void
graphene_ext_quaternion_print (const graphene_quaternion_t *q)
{
  float f[4];
  graphene_ext_quaternion_to_float (q, f);
  g_print ("| %f %f %f %f |\n",
           (double) f[0], (double) f[1], (double) f[2], (double) f[3]);
}

void
graphene_ext_matrix_get_translation_vec3 (const graphene_matrix_t *m,
                                          graphene_vec3_t         *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  graphene_vec3_init (res, f[12], f[13], f[14]);
}

void
graphene_ext_matrix_get_translation_point3d (const graphene_matrix_t *m,
                                             graphene_point3d_t      *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  graphene_point3d_init (res, f[12], f[13], f[14]);
}

void
graphene_ext_matrix_set_translation_vec3 (graphene_matrix_t     *m,
                                          const graphene_vec3_t *t)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  f[12] = graphene_vec3_get_x (t);
  f[13] = graphene_vec3_get_y (t);
  f[14] = graphene_vec3_get_z (t);
  graphene_matrix_init_from_float (m, f);
}

void
graphene_ext_matrix_set_translation_point3d (graphene_matrix_t        *m,
                                             const graphene_point3d_t *t)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  f[12] = t->x;
  f[13] = t->y;
  f[14] = t->z;
  graphene_matrix_init_from_float (m, f);
}

void
graphene_ext_matrix_get_scale (const graphene_matrix_t *m,
                               graphene_vec3_t         *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);

  graphene_vec3_t sx, sy, sz;
  graphene_vec3_init (&sx, f[0], f[1], f[2]);
  graphene_vec3_init (&sy, f[4], f[5], f[6]);
  graphene_vec3_init (&sz, f[8], f[9], f[10]);

  graphene_vec3_init (res,
                      graphene_vec3_length (&sx),
                      graphene_vec3_length (&sy),
                      graphene_vec3_length (&sz));
}

void
graphene_ext_matrix_get_rotation_matrix (const graphene_matrix_t *m,
                                         graphene_matrix_t *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);

  graphene_vec3_t s_vec;
  graphene_ext_matrix_get_scale (m, &s_vec);
  float s[3];
  graphene_vec3_to_float (&s_vec, s);

  float r[16] = {
    f[0] / s[0], f[1] / s[0], f[2]  / s[0], 0,
    f[4] / s[1], f[5] / s[1], f[6]  / s[1], 0,
    f[8] / s[2], f[9] / s[2], f[10] / s[2], 0,
    0,           0,           0,            1
  };
  graphene_matrix_init_from_float (res, r);
}

void
graphene_ext_matrix_get_rotation_quaternion (const graphene_matrix_t *m,
                                             graphene_quaternion_t   *res)
{
  graphene_matrix_t rot_m;
  graphene_ext_matrix_get_rotation_matrix (m, &rot_m);
  graphene_quaternion_init_from_matrix (res, &rot_m);
}

void
graphene_ext_matrix_get_rotation_angles (const graphene_matrix_t *m,
                                         float                   *deg_x,
                                         float                   *deg_y,
                                         float                   *deg_z)
{
  graphene_quaternion_t q;
  graphene_ext_matrix_get_rotation_quaternion (m, &q);
  graphene_quaternion_to_angles (&q, deg_x, deg_y, deg_z);
}

/**
 * graphene_point_scale:
 * @p: a #graphene_point_t
 * @factor: the scaling factor
 * @res: (out caller-allocates): return location for the scaled point
 *
 * Scales the coordinates of the given #graphene_point_t by
 * the given @factor.
 */
void
graphene_ext_point_scale (const graphene_point_t *p,
                          float                   factor,
                          graphene_point_t       *res)
{
  graphene_point_init (res, p->x * factor, p->y * factor);
}

void
graphene_ext_ray_get_origin_vec4 (const graphene_ray_t *r,
                                  float                 w,
                                  graphene_vec4_t      *res)
{
  graphene_vec3_t o;
  graphene_ext_ray_get_origin_vec3 (r, &o);
  graphene_vec4_init_from_vec3 (res, &o, w);
}

void
graphene_ext_ray_get_origin_vec3 (const graphene_ray_t *r,
                                  graphene_vec3_t      *res)
{
  graphene_point3d_t o;
  graphene_ray_get_origin (r, &o);
  graphene_point3d_to_vec3 (&o, res);
}

void
graphene_ext_ray_get_direction_vec4 (const graphene_ray_t *r,
                                     float                 w,
                                     graphene_vec4_t      *res)
{
  graphene_vec3_t d;
  graphene_ray_get_direction (r, &d);
  graphene_vec4_init_from_vec3 (res, &d, w);
}

void
graphene_ext_vec4_print (const graphene_vec4_t *v)
{
  float f[4];
  graphene_vec4_to_float (v, f);
  g_print ("| %f %f %f %f |\n",
           (double) f[0], (double) f[1], (double) f[2], (double) f[3]);
}

void
graphene_ext_vec3_print (const graphene_vec3_t *v)
{
  float f[3];
  graphene_vec3_to_float (v, f);
  g_print ("| %f %f %f |\n", (double) f[0], (double) f[1], (double) f[2]);
}

bool
graphene_ext_matrix_equals (graphene_matrix_t *a,
                            graphene_matrix_t *b)
{
  float a_f[16];
  float b_f[16];

  graphene_matrix_to_float (a, a_f);
  graphene_matrix_to_float (b, b_f);

  for (uint32_t i = 0; i < 16; i++)
    if (a_f[i] != b_f[i])
      return FALSE;

  return TRUE;
}

void
graphene_ext_matrix_interpolate_simple (const graphene_matrix_t *from,
                                        const graphene_matrix_t *to,
                                        float                    factor,
                                        graphene_matrix_t       *result)
{
  float from_f[16];
  float to_f[16];
  float interpolated_f[16];

  graphene_matrix_to_float (from, from_f);
  graphene_matrix_to_float (to, to_f);

  for (uint32_t i = 0; i < 16; i++)
    interpolated_f[i] = from_f[i] * (1.0f - factor) + to_f[i] * factor;

  graphene_matrix_init_from_float (result, interpolated_f);
}

