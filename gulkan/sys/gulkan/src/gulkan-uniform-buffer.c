/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-uniform-buffer.h"
#include "gulkan-buffer.h"

struct _GulkanUniformBuffer
{
  GObject parent;

  GulkanBuffer *buffer;
  VkDevice device;
  VkDeviceSize size;
  void *data;
};

G_DEFINE_TYPE (GulkanUniformBuffer, gulkan_uniform_buffer, G_TYPE_OBJECT)

static void
_finalize (GObject *gobject)
{
  GulkanUniformBuffer *self = GULKAN_UNIFORM_BUFFER (gobject);
  g_object_unref (self->buffer);
}

static void
gulkan_uniform_buffer_class_init (GulkanUniformBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = _finalize;
}

static void
gulkan_uniform_buffer_init (GulkanUniformBuffer *self)
{
  self->buffer = NULL;
}

static gboolean
_allocate_and_map (GulkanUniformBuffer *self,
                   GulkanDevice        *device,
                   VkDeviceSize         size)
{
  self->device = gulkan_device_get_handle (device);
  self->size = size;

  self->buffer = gulkan_buffer_new (device,
                                    size,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

  if (!self->buffer)
    {
      g_printerr ("Could not create buffer\n");
      return FALSE;
    }

  if (!gulkan_buffer_map (self->buffer, &self->data))
    return FALSE;

  return TRUE;
}

GulkanUniformBuffer *
gulkan_uniform_buffer_new (GulkanDevice *device,
                           VkDeviceSize  size)
{
  GulkanUniformBuffer* self =
    (GulkanUniformBuffer*) g_object_new (GULKAN_TYPE_UNIFORM_BUFFER, 0);

  if (!_allocate_and_map (self, device, size))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

void
gulkan_uniform_buffer_update (GulkanUniformBuffer *self,
                              gpointer            *s)
{
  memcpy (self->data, s, self->size);
}

VkBuffer
gulkan_uniform_buffer_get_handle (GulkanUniformBuffer *self)
{
  return gulkan_buffer_get_handle (self->buffer);
}
