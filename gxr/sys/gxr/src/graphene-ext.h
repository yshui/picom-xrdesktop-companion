/*
 * Graphene Extensions
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GRAPHENE_EXT_H_
#define XRD_GRAPHENE_EXT_H_

#include <graphene.h>

void
graphene_ext_quaternion_to_float (const graphene_quaternion_t *q,
                                  float                       *dest);

void
graphene_ext_quaternion_print (const graphene_quaternion_t *q);

void
graphene_ext_matrix_get_translation_vec3 (const graphene_matrix_t *m,
                                          graphene_vec3_t         *res);

void
graphene_ext_matrix_get_translation_point3d (const graphene_matrix_t *m,
                                             graphene_point3d_t      *res);

void
graphene_ext_matrix_set_translation_vec3 (graphene_matrix_t     *m,
                                          const graphene_vec3_t *t);

void
graphene_ext_matrix_set_translation_point3d (graphene_matrix_t        *m,
                                             const graphene_point3d_t *t);

void
graphene_ext_matrix_get_scale (const graphene_matrix_t *m,
                               graphene_vec3_t         *res);

void
graphene_ext_matrix_get_rotation_matrix (const graphene_matrix_t *m,
                                         graphene_matrix_t *res);

void
graphene_ext_matrix_get_rotation_quaternion (const graphene_matrix_t *m,
                                             graphene_quaternion_t   *res);
void
graphene_ext_matrix_get_rotation_angles (const graphene_matrix_t *m,
                                         float                   *deg_x,
                                         float                   *deg_y,
                                         float                   *deg_z);

void
graphene_ext_point_scale (const graphene_point_t *p,
                          float                   factor,
                          graphene_point_t       *res);

void
graphene_ext_ray_get_origin_vec4 (const graphene_ray_t *r,
                                  float                 w,
                                  graphene_vec4_t      *res);

void
graphene_ext_ray_get_origin_vec3 (const graphene_ray_t *r,
                                  graphene_vec3_t      *res);

void
graphene_ext_ray_get_direction_vec4 (const graphene_ray_t *r,
                                     float                 w,
                                     graphene_vec4_t      *res);

void
graphene_ext_vec4_print (const graphene_vec4_t *v);

void
graphene_ext_vec3_print (const graphene_vec3_t *v);

bool
graphene_ext_matrix_equals (graphene_matrix_t *a,
                            graphene_matrix_t *b);

void
graphene_ext_matrix_interpolate_simple (const graphene_matrix_t *from,
                                        const graphene_matrix_t *to,
                                        float                    factor,
                                        graphene_matrix_t       *result);

#endif /* XRD_GRAPHENE_QUATERNION_H_ */
