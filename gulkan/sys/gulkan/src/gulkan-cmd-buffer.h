/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_CMD_BUFFER_H_
#define GULKAN_CMD_BUFFER_H_

#include <glib-object.h>

#include <vulkan/vulkan.h>

G_BEGIN_DECLS

#define GULKAN_TYPE_CMD_BUFFER gulkan_cmd_buffer_get_type()
G_DECLARE_FINAL_TYPE (GulkanCmdBuffer, gulkan_cmd_buffer,
                      GULKAN, CMD_BUFFER, GObject)

gboolean
gulkan_cmd_buffer_begin (GulkanCmdBuffer *self);

VkCommandBuffer
gulkan_cmd_buffer_get_handle (GulkanCmdBuffer *self);

G_END_DECLS

#endif /* GULKAN_CMD_BUFFER_H_ */
