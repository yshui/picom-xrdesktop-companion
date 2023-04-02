/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_FRAME_BUFFER_H_
#define GULKAN_FRAME_BUFFER_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <vulkan/vulkan.h>

#include <gulkan-device.h>
#include <gulkan-texture.h>

#include <glib-object.h>

#include "gulkan-render-pass.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_FRAME_BUFFER gulkan_frame_buffer_get_type()
G_DECLARE_FINAL_TYPE (GulkanFrameBuffer, gulkan_frame_buffer,
                      GULKAN, FRAME_BUFFER, GObject)

GulkanFrameBuffer *
gulkan_frame_buffer_new (GulkanDevice         *device,
                         GulkanRenderPass     *render_pass,
                         VkExtent2D            extent,
                         VkSampleCountFlagBits sample_count,
                         VkFormat              color_format,
                         gboolean              use_depth);

GulkanFrameBuffer *
gulkan_frame_buffer_new_from_image_with_depth (GulkanDevice         *device,
                                               GulkanRenderPass     *render_pass,
                                               VkImage               color_image,
                                               VkExtent2D            extent,
                                               VkSampleCountFlagBits sample_count,
                                               VkFormat              color_format);

GulkanFrameBuffer *
gulkan_frame_buffer_new_from_image (GulkanDevice     *device,
                                    GulkanRenderPass *render_pass,
                                    VkImage           color_image,
                                    VkExtent2D        extent,
                                    VkFormat          color_format);

VkImage
gulkan_frame_buffer_get_color_image (GulkanFrameBuffer *self);

VkFramebuffer
gulkan_frame_buffer_get_handle (GulkanFrameBuffer *self);

G_END_DECLS

#endif /* GULKAN_FRAME_BUFFER_H_ */
