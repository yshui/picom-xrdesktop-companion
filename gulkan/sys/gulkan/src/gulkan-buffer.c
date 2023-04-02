/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-buffer.h"

#include <vulkan/vulkan.h>

struct _GulkanBuffer
{
  GObject parent;

  GulkanDevice *device;

  VkBuffer handle;
  VkDeviceMemory memory;
};

G_DEFINE_TYPE (GulkanBuffer, gulkan_buffer, G_TYPE_OBJECT)

static void
gulkan_buffer_init (GulkanBuffer *self)
{
  self->handle = VK_NULL_HANDLE;
  self->device = VK_NULL_HANDLE;
}

static void
_finalize (GObject *gobject)
{
  GulkanBuffer *self = GULKAN_BUFFER (gobject);
  VkDevice device = gulkan_device_get_handle (self->device);

  if (self->handle != VK_NULL_HANDLE)
    vkDestroyBuffer (device, self->handle, NULL);

  if (self->memory != VK_NULL_HANDLE)
    vkFreeMemory (device, self->memory, NULL);
  G_OBJECT_CLASS (gulkan_buffer_parent_class)->finalize (gobject);
}

static void
gulkan_buffer_class_init (GulkanBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

static VkDeviceSize
_align_to_non_coherent_atom_size (GulkanBuffer *self,
                                  VkDeviceSize allocation_size)
{
  VkPhysicalDeviceProperties *props =
    gulkan_device_get_physical_device_properties (self->device);
  VkDeviceSize atom_size = props->limits.nonCoherentAtomSize;

  VkDeviceSize r = allocation_size % atom_size;
  return r ? allocation_size + (atom_size - r) : allocation_size;
}

static gboolean
_create (GulkanBuffer         *self,
         VkDeviceSize          size,
         VkBufferUsageFlags    usage,
         VkMemoryPropertyFlags properties)
{
  VkExternalMemoryBufferCreateInfo external_info = {
    .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO,
    .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
  };

  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = &external_info,
    .size = size,
    .usage = usage,
  };

  VkDevice device = gulkan_device_get_handle (self->device);

  VkResult res = vkCreateBuffer (device, &buffer_info, NULL, &self->handle);
  vk_check_error ("vkCreateBuffer", res, FALSE);

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements (device, self->handle, &requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = _align_to_non_coherent_atom_size (self, requirements.size)
  };

  if (!gulkan_device_memory_type_from_properties (
        self->device,
        requirements.memoryTypeBits,
        properties,
        &alloc_info.memoryTypeIndex))
  {
    g_printerr ("Failed to find matching memoryTypeIndex for buffer\n");
    return FALSE;
  }

  res = vkAllocateMemory (device, &alloc_info, NULL, &self->memory);
  vk_check_error ("vkAllocateMemory", res, FALSE);

  res = vkBindBufferMemory (device, self->handle, self->memory, 0);
  vk_check_error ("vkBindBufferMemory", res, FALSE);

  return TRUE;
}

GulkanBuffer*
gulkan_buffer_new (GulkanDevice         *device,
                   VkDeviceSize          size,
                   VkBufferUsageFlags    usage,
                   VkMemoryPropertyFlags properties)
{
  GulkanBuffer *self = (GulkanBuffer*) g_object_new (GULKAN_TYPE_BUFFER, 0);
  self->device = device;

  if (!_create (self, size, usage, properties))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

GulkanBuffer*
gulkan_buffer_new_from_data (GulkanDevice         *device,
                             const void           *data,
                             VkDeviceSize          size,
                             VkBufferUsageFlags    usage,
                             VkMemoryPropertyFlags properties)
{
  GulkanBuffer *self = (GulkanBuffer*) g_object_new (GULKAN_TYPE_BUFFER, 0);
  self->device = device;

  if (!_create (self, size, usage, properties))
    {
      g_object_unref (self);
      return NULL;
    }

  if (!gulkan_buffer_upload (self, data, size))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

gboolean
gulkan_buffer_map (GulkanBuffer *self,
                   void        **data)
{
  VkDevice device = gulkan_device_get_handle (self->device);

  VkResult res = vkMapMemory (device, self->memory, 0, VK_WHOLE_SIZE, 0, data);
  vk_check_error ("vkMapMemory", res, FALSE);

  return TRUE;
}

void
gulkan_buffer_unmap (GulkanBuffer *self)
{
  VkDevice device = gulkan_device_get_handle (self->device);
  vkUnmapMemory (device, self->memory);
}

gboolean
gulkan_buffer_upload (GulkanBuffer  *self,
                      const void    *data,
                      VkDeviceSize   size)
{
  if (data == NULL)
    {
      g_printerr ("Trying to upload NULL memory.\n");
      return FALSE;
    }

  VkDevice device = gulkan_device_get_handle (self->device);

  void *tmp;
  if (!gulkan_buffer_map (self, &tmp))
    return FALSE;

  memcpy (tmp, data, size);

  VkMappedMemoryRange memory_range = {
    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
    .memory = self->memory,
    .size = VK_WHOLE_SIZE
  };
  VkResult res = vkFlushMappedMemoryRanges (device, 1, &memory_range);
  vk_check_error ("vkFlushMappedMemoryRanges", res, FALSE);

  gulkan_buffer_unmap (self);

  return TRUE;
}

VkBuffer
gulkan_buffer_get_handle (GulkanBuffer *self)
{
  return self->handle;
}

VkDeviceMemory
gulkan_buffer_get_memory_handle (GulkanBuffer *self)
{
  return self->memory;
}
