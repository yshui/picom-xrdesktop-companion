/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */


#include <gmodule.h>
#include <graphene.h>

#include "gulkan-vertex-buffer.h"
#include "gulkan-buffer.h"

struct _GulkanVertexBuffer
{
  GObject parent;

  VkDevice device;

  GulkanBuffer *buffer;
  GulkanBuffer *index_buffer;

  size_t colors_offset, normals_offset;

  uint32_t count;

  GArray *array;
};

G_DEFINE_TYPE (GulkanVertexBuffer, gulkan_vertex_buffer, G_TYPE_OBJECT)

static void
gulkan_vertex_buffer_finalize (GObject *gobject);

static void
gulkan_vertex_buffer_class_init (GulkanVertexBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gulkan_vertex_buffer_finalize;
}

static void
gulkan_vertex_buffer_init (GulkanVertexBuffer *self)
{
  self->device = VK_NULL_HANDLE;
  self->count = 0;
  self->array = g_array_new (FALSE, FALSE, sizeof (float));
}

GulkanVertexBuffer *
gulkan_vertex_buffer_new (void)
{
  return (GulkanVertexBuffer*) g_object_new (GULKAN_TYPE_VERTEX_BUFFER, 0);
}

static gboolean
_init_from_attribs (GulkanVertexBuffer *self,
                    GulkanDevice       *device,
                    const float        *positions,
                    size_t              positions_size,
                    const float        *colors,
                    size_t              colors_size,
                    const float        *normals,
                    size_t              normals_size)
{
  size_t size = self->normals_offset + normals_size;

  self->buffer = gulkan_buffer_new (device, size,
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!self->buffer)
    return FALSE;

  void *map;
  VkResult res = vkMapMemory (self->device,
                              gulkan_buffer_get_memory_handle (self->buffer),
                              0, positions_size, 0, &map);
  vk_check_error ("vkMapMemory", res, FALSE);

  memcpy (map, positions, positions_size);

  vkUnmapMemory (self->device, gulkan_buffer_get_memory_handle (self->buffer));
  vk_check_error ("vkUnmapMemory", res, FALSE);

  res = vkMapMemory (self->device,
                     gulkan_buffer_get_memory_handle (self->buffer),
                     self->colors_offset, colors_size, 0, &map);
  vk_check_error ("vkMapMemory", res, FALSE);

  memcpy (map, colors, colors_size);

  vkUnmapMemory (self->device, gulkan_buffer_get_memory_handle (self->buffer));
  vk_check_error ("vkUnmapMemory", res, FALSE);

  res = vkMapMemory (self->device,
                     gulkan_buffer_get_memory_handle (self->buffer),
                     self->normals_offset, normals_size, 0, &map);
  vk_check_error ("vkMapMemory", res, FALSE);

  memcpy (map, normals, normals_size);

  vkUnmapMemory (self->device, gulkan_buffer_get_memory_handle (self->buffer));
  vk_check_error ("vkUnmapMemory", res, FALSE);

  return TRUE;
}

GulkanVertexBuffer *
gulkan_vertex_buffer_new_from_attribs (GulkanDevice *device,
                                       const float  *positions,
                                       size_t        positions_size,
                                       const float  *colors,
                                       size_t        colors_size,
                                       const float  *normals,
                                       size_t        normals_size)
{
  GulkanVertexBuffer* self =
    (GulkanVertexBuffer*) g_object_new (GULKAN_TYPE_VERTEX_BUFFER, 0);

  self->device = gulkan_device_get_handle (device);

  self->colors_offset = positions_size;
  self->normals_offset = self->colors_offset + colors_size;

  if (!_init_from_attribs (self, device, positions, positions_size,
                           colors, colors_size, normals, normals_size))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

static void
gulkan_vertex_buffer_finalize (GObject *gobject)
{
  GulkanVertexBuffer *self = GULKAN_VERTEX_BUFFER (gobject);

  g_array_free (self->array, TRUE);

  if (self->device == VK_NULL_HANDLE)
    return;

  g_object_unref (self->buffer);

  if (self->index_buffer)
    g_object_unref (self->index_buffer);
}

void
gulkan_vertex_buffer_draw (GulkanVertexBuffer *self, VkCommandBuffer cmd_buffer)
{
  VkDeviceSize offsets[1] = {0};
  VkBuffer buffer = gulkan_buffer_get_handle (self->buffer);
  vkCmdBindVertexBuffers (cmd_buffer, 0, 1, &buffer, &offsets[0]);
  vkCmdDraw (cmd_buffer, self->count, 1, 0, 0);
}

void
gulkan_vertex_buffer_draw_indexed (GulkanVertexBuffer *self,
                                   VkCommandBuffer cmd_buffer)
{
  VkDeviceSize offsets[1] = {0};
  VkBuffer buffer = gulkan_buffer_get_handle (self->buffer);
  VkBuffer index_buffer = gulkan_buffer_get_handle (self->index_buffer);
  vkCmdBindVertexBuffers (cmd_buffer, 0, 1, &buffer, &offsets[0]);
  vkCmdBindIndexBuffer (cmd_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed (cmd_buffer, self->count, 1, 0, 0, 0);
}

void
gulkan_vertex_buffer_bind_with_offsets (GulkanVertexBuffer *self,
                                        VkCommandBuffer cmd_buffer)
{
  VkBuffer buffer = gulkan_buffer_get_handle (self->buffer);
  vkCmdBindVertexBuffers (
    cmd_buffer, 0, 3,
    (VkBuffer[]){ buffer, buffer, buffer },
    (VkDeviceSize[]){ 0, self->colors_offset, self->normals_offset });
}

void
gulkan_vertex_buffer_reset (GulkanVertexBuffer *self)
{
  g_array_free (self->array, TRUE);

  self->array = g_array_new (FALSE, FALSE, sizeof (float));
  self->count = 0;
}

static void
_append_float (GArray *array, float v)
{
  g_array_append_val (array, v);
}

static void
gulkan_vertex_buffer_append_vec2f (GulkanVertexBuffer *self, float u, float v)
{
  _append_float (self->array, u);
  _append_float (self->array, v);
}

static void
gulkan_vertex_buffer_append_vec3 (GulkanVertexBuffer *self, graphene_vec3_t *v)
{
  _append_float (self->array, graphene_vec3_get_x (v));
  _append_float (self->array, graphene_vec3_get_y (v));
  _append_float (self->array, graphene_vec3_get_z (v));
}

static void
gulkan_vertex_buffer_append_vec4 (GulkanVertexBuffer *self, graphene_vec4_t *v)
{
  _append_float (self->array, graphene_vec4_get_x (v));
  _append_float (self->array, graphene_vec4_get_y (v));
  _append_float (self->array, graphene_vec4_get_z (v));
}

void
gulkan_vertex_buffer_append_with_color (GulkanVertexBuffer *self,
                                        graphene_vec4_t *vec,
                                        graphene_vec3_t *color)
{
  gulkan_vertex_buffer_append_vec4 (self, vec);
  gulkan_vertex_buffer_append_vec3 (self, color);

  self->count++;
}

void
gulkan_vertex_buffer_append_position_uv (GulkanVertexBuffer *self,
                                         graphene_vec4_t *vec,
                                         float u, float v)
{
  gulkan_vertex_buffer_append_vec4 (self, vec);
  gulkan_vertex_buffer_append_vec2f (self, u, v);

  self->count++;
}

gboolean
gulkan_vertex_buffer_alloc_array (GulkanVertexBuffer *self,
                                  GulkanDevice       *device)
{
  self->device = gulkan_device_get_handle (device);
  self->buffer = gulkan_buffer_new_from_data (device, self->array->data,
                                              self->array->len * sizeof (float),
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  return self->buffer != NULL;
}

gboolean
gulkan_vertex_buffer_alloc_data (GulkanVertexBuffer *self,
                                 GulkanDevice       *device,
                                 const void         *data,
                                 VkDeviceSize        size)
{
  self->device = gulkan_device_get_handle (device);
  self->buffer = gulkan_buffer_new_from_data (device, data, size,
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  return self->buffer != NULL;
}

gboolean
gulkan_vertex_buffer_alloc_index_data (GulkanVertexBuffer *self,
                                       GulkanDevice       *device,
                                       const void         *data,
                                       VkDeviceSize        element_size,
                                       guint               element_count)
{
  self->device = gulkan_device_get_handle (device);
  self->index_buffer = gulkan_buffer_new_from_data (device, data,
                                                    element_size * element_count,
                                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  self->count = element_count;

  return self->index_buffer != NULL;
}

gboolean
gulkan_vertex_buffer_alloc_empty (GulkanVertexBuffer *self,
                                  GulkanDevice       *device,
                                  uint32_t            multiplier)
{
  self->device = gulkan_device_get_handle (device);
  if (self->array->len == 0)
    {
      g_printerr ("Vertex array is empty.\n");
      return FALSE;
    }

  self->buffer = gulkan_buffer_new (device,
                                    sizeof (float) * self->array->len * multiplier,
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  return self->buffer != NULL;
}

gboolean
gulkan_vertex_buffer_map_array (GulkanVertexBuffer *self)
{
  if (self->buffer == VK_NULL_HANDLE || self->array->len == 0)
    {
      g_printerr ("Invalid vertex buffer array.\n");
      if (self->buffer == VK_NULL_HANDLE)
        g_printerr ("Buffer is NULL_HANDLE\n");
      if (self->array->len == 0)
        g_printerr ("Array has len 0\n");
      return FALSE;
    }

  if (!gulkan_buffer_upload (self->buffer,
                             self->array->data,
                             self->array->len * sizeof (float)))
    {
      g_printerr ("Could not map memory\n");
      return FALSE;
    }
  return TRUE;
}

gboolean
gulkan_vertex_buffer_is_initialized (GulkanVertexBuffer *self)
{
  return self->buffer != VK_NULL_HANDLE;
}
