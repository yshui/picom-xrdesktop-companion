/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_SYSTEM_H_
#define GXR_SYSTEM_H_

#include <stdint.h>

#include <glib.h>
#include <graphene.h>

#include "gxr-enums.h"
#include "openvr-wrapper.h"
#include "vulkan/vulkan.h"

graphene_matrix_t
openvr_system_get_projection_matrix (GxrEye eye, float near, float far);

graphene_matrix_t
openvr_system_get_eye_to_head_transform (GxrEye eye);

gchar*
openvr_system_get_device_string (TrackedDeviceIndex_t device_index,
                                 ETrackedDeviceProperty property);

GxrEye
openvr_system_eye_to_gxr (EVREye eye);

EVREye
openvr_system_eye_to_openvr (GxrEye eye);

void
openvr_system_get_render_dimensions (VkExtent2D *extent);

void
openvr_system_get_frustum_angles (GxrEye eye,
                                  float *left, float *right,
                                  float *top, float *bottom);

gboolean
openvr_system_get_hmd_pose (graphene_matrix_t *pose);

gboolean
openvr_system_is_input_available (void);

gboolean
openvr_system_is_tracked_device_connected (uint32_t i);

gboolean
openvr_system_device_is_controller (uint32_t i);

gchar*
openvr_system_get_device_model_name (uint32_t i);

#endif /* GXR_SYSTEM_H_ */
