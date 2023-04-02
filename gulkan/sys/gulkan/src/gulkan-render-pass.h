/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_RENDER_PASS_H_
#define GULKAN_RENDER_PASS_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <vulkan/vulkan.h>

#include <gulkan-device.h>
#include <gulkan-texture.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GulkanFrameBuffer GulkanFrameBuffer;

#define GULKAN_TYPE_RENDER_PASS gulkan_render_pass_get_type()
G_DECLARE_FINAL_TYPE (GulkanRenderPass, gulkan_render_pass,
                      GULKAN, RENDER_PASS, GObject)

GulkanRenderPass *
gulkan_render_pass_new (GulkanDevice         *device,
                        VkSampleCountFlagBits samples,
                        VkFormat              color_format,
                        VkImageLayout         final_color_layout,
                        gboolean              use_depth);

void
gulkan_render_pass_begin (GulkanRenderPass  *self,
                          VkExtent2D         extent,
                          VkClearColorValue  clear_color,
                          GulkanFrameBuffer *frame_buffer,
                          VkCommandBuffer    cmd_buffer);

VkRenderPass
gulkan_render_pass_get_handle (GulkanRenderPass *self);

G_END_DECLS

#endif /* GULKAN_RENDER_PASS_H_ */
