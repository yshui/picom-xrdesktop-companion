/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_SWAPCHAIN_H_
#define GULKAN_SWAPCHAIN_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>

#include <vulkan/vulkan.h>

#include "gulkan-client.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_SWAPCHAIN gulkan_swapchain_get_type()
G_DECLARE_FINAL_TYPE (GulkanSwapchain, gulkan_swapchain,
                      GULKAN, SWAPCHAIN, GObject)

GulkanSwapchain *
gulkan_swapchain_new (GulkanClient *client,
                      VkSurfaceKHR surface,
                      VkPresentModeKHR present_mode,
                      VkFormat format,
                      VkColorSpaceKHR colorspace);

uint32_t
gulkan_swapchain_get_size (GulkanSwapchain *self);

VkFormat
gulkan_swapchain_get_format (GulkanSwapchain *self);

gboolean
gulkan_swapchain_acquire (GulkanSwapchain *self,
                          VkSemaphore signal_semaphore,
                          uint32_t *index);

gboolean
gulkan_swapchain_present (GulkanSwapchain *self,
                          VkSemaphore *wait_semaphore,
                          uint32_t index);

void
gulkan_swapchain_get_images (GulkanSwapchain *self,
                             VkImage *swap_chain_images);

VkExtent2D
gulkan_swapchain_get_extent (GulkanSwapchain *self);

gboolean
gulkan_swapchain_reset_surface (GulkanSwapchain *self,
                                VkSurfaceKHR     surface);

G_END_DECLS

#endif /* GULKAN_SWAPCHAIN_H_ */
