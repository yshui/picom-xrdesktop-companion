/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_COMPOSITOR_H_
#define GXR_COMPOSITOR_H_

#include <glib-object.h>

#include <vulkan/vulkan.h>
#include <gulkan.h>

#include "gxr-types.h"

#include "openvr-wrapper.h"
#include "openvr-context.h"

G_BEGIN_DECLS

void
openvr_compositor_wait_get_poses (GxrPose *poses, uint32_t count);

enum ETrackingUniverseOrigin
openvr_compositor_get_tracking_space (void);

gboolean
openvr_compositor_submit (GulkanClient         *client,
                          uint32_t              width,
                          uint32_t              height,
                          VkFormat              format,
                          VkSampleCountFlagBits sample_count,
                          VkImage               left,
                          VkImage               right);


G_END_DECLS

#endif /* GXR_COMPOSITOR_H_ */
