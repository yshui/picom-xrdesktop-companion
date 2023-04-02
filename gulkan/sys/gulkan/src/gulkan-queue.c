/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-queue.h"
#include "gulkan-device.h"
#include "gulkan-cmd-buffer-private.h"

struct _GulkanQueue
{
  GObject parent;

  uint32_t family_index;

  VkQueue handle;
  VkFence fence;

  GulkanDevice *device;

  VkCommandPool pool;

  GMutex pool_mutex;
  GMutex queue_mutex;
};

G_DEFINE_TYPE (GulkanQueue, gulkan_queue, G_TYPE_OBJECT)

static void
gulkan_queue_init (GulkanQueue *self)
{
  self->handle = VK_NULL_HANDLE;
  self->pool = VK_NULL_HANDLE;
  g_mutex_init (&self->pool_mutex);
  g_mutex_init (&self->queue_mutex);
}

GMutex *
gulkan_queue_get_pool_mutex (GulkanQueue *self) {
  return &self->pool_mutex;
}

static gboolean
_init_pool (GulkanQueue *self)
{
  VkDevice vk_device = gulkan_device_get_handle (self->device);

  VkCommandPoolCreateInfo command_pool_info =
    {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = self->family_index,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

  VkResult res = vkCreateCommandPool (vk_device, &command_pool_info, NULL,
                                      &self->pool);
  vk_check_error ("vkCreateCommandPool", res, FALSE);
  return TRUE;
}

GulkanQueue *
gulkan_queue_new (GulkanDevice *device, uint32_t family_index)
{
  GulkanQueue *self = g_object_new (GULKAN_TYPE_QUEUE, 0);
  self->device = device;
  self->family_index = family_index;

  return self;
}

static void
_finalize (GObject *gobject)
{
  GulkanQueue *self = GULKAN_QUEUE (gobject);

  g_mutex_clear (&self->pool_mutex);
  g_mutex_clear (&self->queue_mutex);

  VkDevice device = gulkan_device_get_handle (self->device);

  vkDestroyFence (device, self->fence, NULL);

  if (self->pool != VK_NULL_HANDLE)
    vkDestroyCommandPool (device, self->pool, NULL);

  G_OBJECT_CLASS (gulkan_queue_parent_class)->finalize (gobject);
}

static void
gulkan_queue_class_init (GulkanQueueClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

VkCommandPool
gulkan_queue_get_command_pool (GulkanQueue *self)
{
  return self->pool;
}

gboolean
gulkan_queue_supports_surface (GulkanQueue *self,
                               VkSurfaceKHR surface)
{
  VkBool32 surface_support = FALSE;

  VkPhysicalDevice device = gulkan_device_get_physical_handle (self->device);
  vkGetPhysicalDeviceSurfaceSupportKHR (device,
                                        self->family_index,
                                        surface, &surface_support);
  return (gboolean) surface_support;
}

uint32_t
gulkan_queue_get_family_index (GulkanQueue *self)
{
  return self->family_index;
}

VkQueue
gulkan_queue_get_handle (GulkanQueue *self)
{
  return self->handle;
}

gboolean
gulkan_queue_initialize (GulkanQueue *self)
{
  VkDevice device = gulkan_device_get_handle (self->device);
  vkGetDeviceQueue (device, self->family_index, 0, &self->handle);

  if (!_init_pool(self))
    {
      g_printerr ("Failed to create command pool.\n");
      g_object_unref (self);
      return FALSE;
    }

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  VkResult res = vkCreateFence (device, &fence_info, NULL, &self->fence);
  vk_check_error ("vkCreateFence", res, FALSE);

  return TRUE;
}

GulkanCmdBuffer *
gulkan_queue_request_cmd_buffer (GulkanQueue *self)
{
  g_mutex_lock (&self->pool_mutex);
  GulkanCmdBuffer *cmd_buffer = gulkan_cmd_buffer_new (self->device, self);
  g_mutex_unlock (&self->pool_mutex);
  return cmd_buffer;
}

void
gulkan_queue_free_cmd_buffer (GulkanQueue *self,
                              GulkanCmdBuffer *cmd_buffer)
{
  g_mutex_lock (&self->pool_mutex);
  g_object_unref (cmd_buffer);
  g_mutex_unlock (&self->pool_mutex);
}

gboolean
gulkan_queue_submit (GulkanQueue *self, GulkanCmdBuffer *cmd_buffer)
{
  VkCommandBuffer cmd_buffer_handle = gulkan_cmd_buffer_get_handle (cmd_buffer);
  if (self->handle == VK_NULL_HANDLE)
    {
      g_printerr ("Trying to submit empty command buffer\n.");
      return FALSE;
    }

  VkResult res = vkEndCommandBuffer (cmd_buffer_handle);
  vk_check_error ("vkEndCommandBuffer", res, FALSE);

  g_mutex_lock (&self->queue_mutex);
  VkDevice d = gulkan_device_get_handle (self->device);
  res = vkWaitForFences (d, 1, &self->fence, VK_TRUE, UINT64_MAX);
  res = vkResetFences (d, 1, &self->fence);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &cmd_buffer_handle
  };

  res = vkQueueSubmit (self->handle, 1, &submit_info, self->fence);

  vkQueueWaitIdle (self->handle);

  g_mutex_unlock (&self->queue_mutex);
  vk_check_error ("vkQueueSubmit", res, FALSE);


  return TRUE;
}
