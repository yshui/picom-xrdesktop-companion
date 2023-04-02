/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-cmd-buffer-private.h"
#include "gulkan-client.h"

/**
 * _GulkanCmdBuffer:
 * @handle: The #VkCommandBuffer handle of the buffer.
 * @fence: A #VkFence used with the buffer.
 *
 * Structure that contains a command buffer handle and a fence.
 **/
struct _GulkanCmdBuffer
{
  GObject parent;

  VkCommandBuffer handle;
  VkDevice device;
  VkQueue queue;

  VkCommandPool pool;
};

G_DEFINE_TYPE (GulkanCmdBuffer, gulkan_cmd_buffer, G_TYPE_OBJECT)

static void
gulkan_cmd_buffer_init (GulkanCmdBuffer *self)
{
  (void) self;
}

static GulkanCmdBuffer *
_new (void)
{
  return (GulkanCmdBuffer*) g_object_new (GULKAN_TYPE_CMD_BUFFER, 0);
}

static void
_finalize (GObject *gobject)
{
  GulkanCmdBuffer *self = GULKAN_CMD_BUFFER (gobject);
  vkFreeCommandBuffers (self->device, self->pool, 1, &self->handle);
  G_OBJECT_CLASS (gulkan_cmd_buffer_parent_class)->finalize (gobject);
}

static void
gulkan_cmd_buffer_class_init (GulkanCmdBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

GulkanCmdBuffer *gulkan_cmd_buffer_new (GulkanDevice *device,
                                        GulkanQueue  *queue) {
  GulkanCmdBuffer *self = _new();
  self->pool = gulkan_queue_get_command_pool (queue);
  self->device = gulkan_device_get_handle (device);
  self->queue = gulkan_queue_get_handle (queue);

  VkCommandBufferAllocateInfo command_buffer_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandBufferCount = 1,
    .commandPool = self->pool,
  };

  VkResult res;
  res = vkAllocateCommandBuffers (self->device, &command_buffer_info,
                                 &self->handle);
  vk_check_error ("vkAllocateCommandBuffers", res, NULL);

  return self;
}

gboolean
gulkan_cmd_buffer_begin (GulkanCmdBuffer *self)
{
  VkResult res;

  VkCommandBufferBeginInfo command_buffer_begin_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  res = vkBeginCommandBuffer (self->handle,
                             &command_buffer_begin_info);
  vk_check_error ("vkBeginCommandBuffer", res, FALSE);

  return TRUE;
}

VkCommandBuffer
gulkan_cmd_buffer_get_handle (GulkanCmdBuffer *self)
{
  return self->handle;
}
