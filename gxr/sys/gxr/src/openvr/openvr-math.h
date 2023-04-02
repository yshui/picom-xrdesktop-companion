/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef openvr_math_H_
#define openvr_math_H_

#include "openvr-wrapper.h"
#include <graphene.h>
#include <glib.h>

void
openvr_math_print_matrix34 (HmdMatrix34_t mat);

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34);

gboolean
openvr_math_pose_to_matrix (TrackedDevicePose_t *pose,
                            graphene_matrix_t   *transform);

void
openvr_math_matrix34_to_graphene (HmdMatrix34_t *mat34, graphene_matrix_t *mat);

void
openvr_math_matrix44_to_graphene (HmdMatrix44_t *mat44, graphene_matrix_t *mat);

#endif /* openvr_math_H_ */
