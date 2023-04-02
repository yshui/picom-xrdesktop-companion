/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-background.h"
#include <gulkan.h>
#include "graphene-ext.h"

typedef struct __attribute__((__packed__)) {
  float mvp[16];
} XrdSceneBackgroundUniformBuffer;

struct _XrdSceneBackground
{
  XrdSceneObject parent;
  GulkanVertexBuffer *vertex_buffer;
};

G_DEFINE_TYPE (XrdSceneBackground, xrd_scene_background, XRD_TYPE_SCENE_OBJECT)

static void
xrd_scene_background_finalize (GObject *gobject);

static void
xrd_scene_background_class_init (XrdSceneBackgroundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_background_finalize;
}

static void
xrd_scene_background_init (XrdSceneBackground *self)
{
  self->vertex_buffer = gulkan_vertex_buffer_new ();
}

static gboolean
_initialize (XrdSceneBackground    *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout);

XrdSceneBackground *
xrd_scene_background_new (GulkanClient          *gulkan,
                          VkDescriptorSetLayout *layout)
{
  XrdSceneBackground *self =
    (XrdSceneBackground*) g_object_new (XRD_TYPE_SCENE_BACKGROUND, 0);

  _initialize (self, gulkan, layout);

  return self;
}

static void
xrd_scene_background_finalize (GObject *gobject)
{
  XrdSceneBackground *self = XRD_SCENE_BACKGROUND (gobject);
  g_clear_object (&self->vertex_buffer);
  G_OBJECT_CLASS (xrd_scene_background_parent_class)->finalize (gobject);
}

static void
_append_star (GulkanVertexBuffer *self,
              float               radius,
              float               y,
              uint32_t            sections,
              graphene_vec3_t    *color)
{
  graphene_vec4_t *points = g_malloc (sizeof(graphene_vec4_t) * sections);

  graphene_vec4_init (&points[0],  radius, y, 0, 1);
  graphene_vec4_init (&points[1], -radius, y, 0, 1);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_y (&rotation, 360.0f / (float) sections);

  for (uint32_t i = 0; i < sections / 2 - 1; i++)
    {
      uint32_t j = i * 2;
      graphene_matrix_transform_vec4 (&rotation, &points[j],     &points[j + 2]);
      graphene_matrix_transform_vec4 (&rotation, &points[j + 1], &points[j + 3]);
    }

  for (uint32_t i = 0; i < sections; i++)
    gulkan_vertex_buffer_append_with_color (self, &points[i], color);

  g_free (points);
}

static void
_append_circle (GulkanVertexBuffer *self,
                float               radius,
                float               y,
                uint32_t            edges,
                graphene_vec3_t    *color)
{
  graphene_vec4_t *points = g_malloc (sizeof(graphene_vec4_t) * edges * 2);

  graphene_vec4_init (&points[0], radius, y, 0, 1);

  graphene_matrix_t rotation;
  graphene_matrix_init_identity (&rotation);
  graphene_matrix_rotate_y (&rotation, 360.0f / (float) edges);

  for (uint32_t i = 0; i < edges; i++)
    {
      uint32_t j = i * 2;
      if (i != 0)
        graphene_vec4_init_from_vec4 (&points[j], &points[j - 1]);
      graphene_matrix_transform_vec4 (&rotation, &points[j], &points[j + 1]);
    }

  for (uint32_t i = 0; i < edges * 2; i++)
    gulkan_vertex_buffer_append_with_color (self, &points[i], color);

  g_free(points);
}

static void
_append_floor (GulkanVertexBuffer *self,
               uint32_t            radius,
               float               y,
               graphene_vec3_t    *color)
{
  _append_star (self, (float) radius, y, 8, color);

  for (uint32_t i = 1; i <= radius; i++)
    _append_circle (self, (float) i, y, 128, color);
}

static gboolean
_initialize (XrdSceneBackground    *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout)
{
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec3_t color;
  graphene_vec3_init (&color, .6f, .6f, .6f);

  _append_floor (self->vertex_buffer, 20, 0.0f, &color);
  _append_floor (self->vertex_buffer, 20, 4.0f, &color);

  GulkanDevice *device = gulkan_client_get_device (gulkan);
  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer, device,
                                         GXR_DEVICE_INDEX_MAX))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (XrdSceneBackgroundUniformBuffer);
  if (!xrd_scene_object_initialize (obj, gulkan, layout, ub_size))
    return FALSE;

  xrd_scene_object_update_descriptors (obj);

  return TRUE;
}

static void
_update_ubo (XrdSceneBackground *self,
             GxrEye              eye,
             graphene_matrix_t  *vp)
{
  XrdSceneBackgroundUniformBuffer ub;

  graphene_matrix_t m_matrix;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&m_matrix, vp, &mvp_matrix);

  float mvp[16];
  graphene_matrix_to_float (&mvp_matrix, mvp);
  for (int i = 0; i < 16; i++)
    ub.mvp[i] = mvp[i];

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_background_render (XrdSceneBackground *self,
                             GxrEye              eye,
                             VkPipeline          pipeline,
                             VkPipelineLayout    pipeline_layout,
                             VkCommandBuffer     cmd_buffer,
                             graphene_matrix_t  *vp)
{
  if (!gulkan_vertex_buffer_is_initialized (self->vertex_buffer))
    return;

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  _update_ubo (self, eye, vp);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}
