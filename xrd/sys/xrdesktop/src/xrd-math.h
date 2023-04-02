/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_MATH_H_
#define XRD_MATH_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#define M_PI 3.14159265358979323846

#include <graphene.h>
#include <glib.h>
#include "xrd-window.h"

float
xrd_math_point_matrix_distance (graphene_point3d_t *intersection_point,
                                graphene_matrix_t  *pose);

void
xrd_math_get_rotation_angles (graphene_vec3_t *direction,
                              float *azimuth,
                              float *inclination);

void
xrd_math_matrix_set_translation_point (graphene_matrix_t  *matrix,
                                       graphene_point3d_t *point);

void
xrd_math_matrix_set_translation_vec (graphene_matrix_t  *matrix,
                                     graphene_vec3_t *vec);

gboolean
xrd_math_intersect_lines_2d (graphene_point_t *p0, graphene_point_t *p1,
                             graphene_point_t *p2, graphene_point_t *p3,
                             graphene_point_t *intersection);

gboolean
xrd_math_clamp_towards_zero_2d (graphene_point_t *min,
                                graphene_point_t *max,
                                graphene_point_t *point,
                                graphene_point_t *clamped);

void
xrd_math_sphere_to_3d_coords (float azimuth,
                              float inclination,
                              float distance,
                              graphene_point3d_t *point);

#endif /* XRD_MATH_H_ */
