/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_QUEUE_H_
#define GULKAN_QUEUE_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>

#include <vulkan/vulkan.h>

#include "gulkan-cmd-buffer.h"

G_BEGIN_DECLS

typedef struct _GulkanDevice GulkanDevice;

#define GULKAN_TYPE_QUEUE gulkan_queue_get_type()
G_DECLARE_FINAL_TYPE (GulkanQueue, gulkan_queue, GULKAN, QUEUE, GObject)

GulkanQueue *
gulkan_queue_new (GulkanDevice *device, uint32_t family_index);

VkCommandPool
gulkan_queue_get_command_pool (GulkanQueue *self);

gboolean
gulkan_queue_supports_surface (GulkanQueue *self,
                               VkSurfaceKHR  surface);

uint32_t
gulkan_queue_get_family_index (GulkanQueue *self);

VkQueue
gulkan_queue_get_handle (GulkanQueue *self);

gboolean
gulkan_queue_initialize (GulkanQueue *self);

GulkanCmdBuffer *
gulkan_queue_request_cmd_buffer (GulkanQueue *self);
void
gulkan_queue_free_cmd_buffer (GulkanQueue *self,
                              GulkanCmdBuffer *cmd_buffer);

gboolean
gulkan_queue_submit (GulkanQueue *self, GulkanCmdBuffer *cmd_buffer);

GMutex *
gulkan_queue_get_pool_mutex (GulkanQueue *self);

G_END_DECLS

#endif /* GULKAN_QUEUE_H_ */
