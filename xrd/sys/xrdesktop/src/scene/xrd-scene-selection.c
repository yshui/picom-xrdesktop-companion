/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-selection.h"
#include <gulkan.h>

typedef struct __attribute__((__packed__)) {
  float mvp[16];
} XrdSceneSelectionUniformBuffer;

struct _XrdSceneSelection
{
  XrdSceneObject parent;
  GulkanVertexBuffer *vertex_buffer;
};

G_DEFINE_TYPE (XrdSceneSelection, xrd_scene_selection, XRD_TYPE_SCENE_OBJECT)

static void
xrd_scene_selection_finalize (GObject *gobject);

static void
xrd_scene_selection_class_init (XrdSceneSelectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_selection_finalize;
}

static void
xrd_scene_selection_init (XrdSceneSelection *self)
{
  self->vertex_buffer = gulkan_vertex_buffer_new ();
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  xrd_scene_object_hide (obj);
}

gboolean
_initialize (XrdSceneSelection     *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout);

XrdSceneSelection *
xrd_scene_selection_new (GulkanClient          *gulkan,
                         VkDescriptorSetLayout *layout)
{
  XrdSceneSelection *self =
    (XrdSceneSelection*) g_object_new (XRD_TYPE_SCENE_SELECTION, 0);

  _initialize (self, gulkan, layout);

  return self;
}

static void
xrd_scene_selection_finalize (GObject *gobject)
{
  XrdSceneSelection *self = XRD_SCENE_SELECTION (gobject);
  g_object_unref (self->vertex_buffer);
  G_OBJECT_CLASS (xrd_scene_selection_parent_class)->finalize (gobject);
}

static void
_append_lines_quad (GulkanVertexBuffer *self,
                    float               aspect_ratio,
                    graphene_vec3_t    *color)
{
  float scale_x = aspect_ratio;
  float scale_y = 1.0f;

  graphene_vec4_t a, b, c, d;
  graphene_vec4_init (&a, -scale_x/2.0f, -scale_y / 2, 0, 1);
  graphene_vec4_init (&b,  scale_x/2.0f, -scale_y / 2, 0, 1);
  graphene_vec4_init (&c,  scale_x/2.0f,  scale_y / 2, 0, 1);
  graphene_vec4_init (&d, -scale_x/2.0f,  scale_y / 2, 0, 1);

  graphene_vec4_t points[8] = {
    a, b, b, c, c, d, d, a
  };

  for (uint32_t i = 0; i < G_N_ELEMENTS (points); i++)
    {
      gulkan_vertex_buffer_append_with_color (self, &points[i], color);
    }
}

void
xrd_scene_selection_set_aspect_ratio (XrdSceneSelection *self,
                                      float              aspect_ratio)
{
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec3_t color;
  graphene_vec3_init (&color, .078f, .471f, .675f);

  _append_lines_quad (self->vertex_buffer, aspect_ratio, &color);

  gulkan_vertex_buffer_map_array (self->vertex_buffer);
}

gboolean
_initialize (XrdSceneSelection     *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout)
{
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec3_t color;
  graphene_vec3_init (&color, .078f, .471f, .675f);

  _append_lines_quad (self->vertex_buffer, 1.0f, &color);

  GulkanDevice *device = gulkan_client_get_device (gulkan);
  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer, device,
                                         GXR_DEVICE_INDEX_MAX))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (XrdSceneSelectionUniformBuffer);
  if (!xrd_scene_object_initialize (obj, gulkan, layout, ub_size))
    return FALSE;

  xrd_scene_object_update_descriptors (obj);

  return TRUE;
}

static void
_update_ubo (XrdSceneSelection *self,
             GxrEye             eye,
             graphene_matrix_t *vp)
{
  XrdSceneSelectionUniformBuffer ub;

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
xrd_scene_selection_render (XrdSceneSelection *self,
                            GxrEye             eye,
                            VkPipeline         pipeline,
                            VkPipelineLayout   pipeline_layout,
                            VkCommandBuffer    cmd_buffer,
                            graphene_matrix_t *vp)
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
