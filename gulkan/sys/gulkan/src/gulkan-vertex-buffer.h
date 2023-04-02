/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_VERTEX_BUFFER_H_
#define GULKAN_VERTEX_BUFFER_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>

#include <vulkan/vulkan.h>

#include <graphene.h>
#include <gulkan-device.h>

G_BEGIN_DECLS

#define GULKAN_TYPE_VERTEX_BUFFER gulkan_vertex_buffer_get_type()
G_DECLARE_FINAL_TYPE (GulkanVertexBuffer, gulkan_vertex_buffer,
                      GULKAN, VERTEX_BUFFER, GObject)

GulkanVertexBuffer *gulkan_vertex_buffer_new (void);

#define GULKAN_VERTEX_BUFFER_NEW_FROM_ATTRIBS(a, b, c, d) \
  gulkan_vertex_buffer_new_from_attribs (a, \
                                         b, sizeof (b), \
                                         c, sizeof (c), \
                                         d, sizeof (d))

GulkanVertexBuffer *
gulkan_vertex_buffer_new_from_attribs (GulkanDevice *device,
                                       const float  *positions,
                                       size_t        positions_size,
                                       const float  *colors,
                                       size_t        colors_size,
                                       const float  *normals,
                                       size_t        normals_size);

void
gulkan_vertex_buffer_draw (GulkanVertexBuffer *self,
                           VkCommandBuffer cmd_buffer);

void
gulkan_vertex_buffer_draw_indexed (GulkanVertexBuffer *self,
                                   VkCommandBuffer cmd_buffer);

void
gulkan_vertex_buffer_reset (GulkanVertexBuffer *self);

gboolean
gulkan_vertex_buffer_map_array (GulkanVertexBuffer *self);

gboolean
gulkan_vertex_buffer_alloc_empty (GulkanVertexBuffer *self,
                                  GulkanDevice       *device,
                                  uint32_t            multiplier);

gboolean
gulkan_vertex_buffer_alloc_array (GulkanVertexBuffer *self,
                                  GulkanDevice       *device);

gboolean
gulkan_vertex_buffer_alloc_data (GulkanVertexBuffer *self,
                                 GulkanDevice       *device,
                                 const void         *data,
                                 VkDeviceSize        size);

gboolean
gulkan_vertex_buffer_alloc_index_data (GulkanVertexBuffer *self,
                                       GulkanDevice       *device,
                                       const void         *data,
                                       VkDeviceSize        element_size,
                                       guint               element_count);

void
gulkan_vertex_buffer_append_position_uv (GulkanVertexBuffer *self,
                                         graphene_vec4_t *vec,
                                         float u, float v);

void
gulkan_vertex_buffer_append_with_color (GulkanVertexBuffer *self,
                                        graphene_vec4_t *vec,
                                        graphene_vec3_t *color);

gboolean
gulkan_vertex_buffer_is_initialized (GulkanVertexBuffer *self);

void
gulkan_vertex_buffer_bind_with_offsets (GulkanVertexBuffer *self,
                                        VkCommandBuffer cmd_buffer);

G_END_DECLS

#endif /* GULKAN_VERTEX_BUFFER_H_ */
