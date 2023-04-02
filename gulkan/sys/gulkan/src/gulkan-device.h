/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_DEVICE_H_
#define GULKAN_DEVICE_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>

#include <vulkan/vulkan.h>

#include "gulkan-instance.h"
#include "gulkan-queue.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_DEVICE gulkan_device_get_type()
G_DECLARE_FINAL_TYPE (GulkanDevice, gulkan_device,
                      GULKAN, DEVICE, GObject)

GulkanDevice *gulkan_device_new (void);

gboolean
gulkan_device_create (GulkanDevice    *self,
                      GulkanInstance  *instance,
                      VkPhysicalDevice device,
                      GSList          *extensions);

gboolean
gulkan_device_memory_type_from_properties (
  GulkanDevice         *self,
  uint32_t              memory_type_bits,
  VkMemoryPropertyFlags memory_property_flags,
  uint32_t             *type_index_out);



VkDevice
gulkan_device_get_handle (GulkanDevice *self);

VkPhysicalDevice
gulkan_device_get_physical_handle (GulkanDevice *self);

gboolean
gulkan_device_get_memory_fd (GulkanDevice  *self,
                             VkDeviceMemory image_memory,
                             int           *fd);

void
gulkan_device_wait_idle (GulkanDevice *self);

void
gulkan_device_print_memory_properties (GulkanDevice *self);

void
gulkan_device_print_memory_budget (GulkanDevice *self);

VkDeviceSize
gulkan_device_get_heap_budget (GulkanDevice *self, uint32_t i);

GulkanQueue*
gulkan_device_get_graphics_queue (GulkanDevice *self);

GulkanQueue*
gulkan_device_get_transfer_queue (GulkanDevice *self);

VkPhysicalDeviceProperties *
gulkan_device_get_physical_device_properties (GulkanDevice *self);

G_END_DECLS

#endif /* GULKAN_DEVICE_H_ */
