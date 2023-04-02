/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_MODEL_H_
#define GXR_MODEL_H_

#include <glib.h>
#include <gulkan.h>

G_BEGIN_DECLS

void
openvr_model_print_list (void);

GSList *
openvr_model_get_list (void);

gboolean
openvr_model_load (GulkanClient       *gc,
                   GulkanVertexBuffer *vbo,
                   GulkanTexture      **texture,
                   VkSampler          *sampler,
                   const char         *model_name);

uint32_t
openvr_model_get_vertex_stride (void);

uint32_t
openvr_model_get_normal_offset (void);

uint32_t
openvr_model_get_uv_offset (void);

G_END_DECLS

#endif /* GXR_MODEL_H_ */
