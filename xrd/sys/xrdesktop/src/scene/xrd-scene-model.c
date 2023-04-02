/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-model.h"

#include <gxr.h>

typedef struct {
  float mvp[16];
} XrdSceneModelUniformBuffer;

struct _XrdSceneModel
{
  XrdSceneObject parent;

  GulkanClient *gulkan;

  GulkanTexture *texture;
  GulkanVertexBuffer *vbo;
  VkSampler sampler;

  gchar *model_name;
};

static gchar *
_get_name (GxrModel *self)
{
  XrdSceneModel *scene_model = XRD_SCENE_MODEL (self);
  return scene_model->model_name;
}


static void
xrd_scene_model_interface_init (GxrModelInterface *iface)
{
  iface->get_name = _get_name;
}

G_DEFINE_TYPE_WITH_CODE (XrdSceneModel, xrd_scene_model, XRD_TYPE_SCENE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_MODEL,
                                                xrd_scene_model_interface_init))

static void
xrd_scene_model_finalize (GObject *gobject);

static void
xrd_scene_model_class_init (XrdSceneModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_model_finalize;
}

static void
xrd_scene_model_init (XrdSceneModel *self)
{
  self->sampler = VK_NULL_HANDLE;
  self->vbo = gulkan_vertex_buffer_new ();
}

XrdSceneModel *
xrd_scene_model_new (VkDescriptorSetLayout *layout, GulkanClient *gulkan)
{
  XrdSceneModel *self = (XrdSceneModel*) g_object_new (XRD_TYPE_SCENE_MODEL, 0);

  self->gulkan = g_object_ref (gulkan);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  VkDeviceSize ub_size = sizeof (XrdSceneModelUniformBuffer);
  if (!xrd_scene_object_initialize (obj, gulkan, layout, ub_size))
    return FALSE;

  return self;
}

static void
xrd_scene_model_finalize (GObject *gobject)
{
  XrdSceneModel *self = XRD_SCENE_MODEL (gobject);
  g_free (self->model_name);
  g_clear_object (&self->vbo);
  g_clear_object (&self->texture);

  GulkanDevice *device = gulkan_client_get_device (self->gulkan);

  if (self->sampler != VK_NULL_HANDLE)
    vkDestroySampler (gulkan_device_get_handle (device),
                      self->sampler, NULL);

  g_clear_object (&self->gulkan);

  G_OBJECT_CLASS (xrd_scene_model_parent_class)->finalize (gobject);
}

VkSampler
xrd_scene_model_get_sampler (XrdSceneModel *self)
{
  return self->sampler;
}

GulkanVertexBuffer*
xrd_scene_model_get_vbo (XrdSceneModel *self)
{
  return self->vbo;
}

GulkanTexture*
xrd_scene_model_get_texture (XrdSceneModel *self)
{
  return self->texture;
}

gboolean
xrd_scene_model_load (XrdSceneModel *self,
                      GxrContext    *context,
                      const char    *model_name)
{
  gboolean res =
    gxr_context_load_model (context, self->vbo, &self->texture,
                            &self->sampler, model_name);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  xrd_scene_object_update_descriptors_texture (
    obj, self->sampler,
    gulkan_texture_get_image_view (self->texture));

  self->model_name = g_strdup (model_name);
  return res;
}

static void
_update_ubo (XrdSceneModel     *self,
             GxrEye             eye,
             graphene_matrix_t *transformation,
             graphene_matrix_t *vp)
{
  XrdSceneModelUniformBuffer ub;

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (transformation, vp, &mvp_matrix);
  graphene_matrix_to_float (&mvp_matrix, ub.mvp);

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_model_render (XrdSceneModel     *self,
                        GxrEye             eye,
                        VkPipeline         pipeline,
                        VkCommandBuffer    cmd_buffer,
                        VkPipelineLayout   pipeline_layout,
                        graphene_matrix_t *transformation,
                        graphene_matrix_t *vp)
{
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  _update_ubo (self, eye, transformation, vp);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw_indexed (self->vbo, cmd_buffer);
}
