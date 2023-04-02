/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_BUFFER_H_
#define GULKAN_BUFFER_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>
#include "gulkan-device.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_BUFFER gulkan_buffer_get_type()
G_DECLARE_FINAL_TYPE (GulkanBuffer, gulkan_buffer,
                      GULKAN, BUFFER, GObject)

GulkanBuffer*
gulkan_buffer_new (GulkanDevice         *device,
                   VkDeviceSize          size,
                   VkBufferUsageFlags    usage,
                   VkMemoryPropertyFlags properties);

GulkanBuffer*
gulkan_buffer_new_from_data (GulkanDevice         *device,
                             const void           *data,
                             VkDeviceSize          size,
                             VkBufferUsageFlags    usage,
                             VkMemoryPropertyFlags properties);

gboolean
gulkan_buffer_map (GulkanBuffer *self,
                   void        **data);

void
gulkan_buffer_unmap (GulkanBuffer *self);

gboolean
gulkan_buffer_upload (GulkanBuffer *self,
                      const void   *data,
                      VkDeviceSize  size);

VkBuffer
gulkan_buffer_get_handle (GulkanBuffer *self);

VkDeviceMemory
gulkan_buffer_get_memory_handle (GulkanBuffer *self);

G_END_DECLS

#endif /* GULKAN_BUFFER_H_ */
