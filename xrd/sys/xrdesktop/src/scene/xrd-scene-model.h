/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_MODEL_H_
#define XRD_SCENE_MODEL_H_

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

#include "xrd-scene-object.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_MODEL xrd_scene_model_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneModel, xrd_scene_model,
                      XRD, SCENE_MODEL, XrdSceneObject)

XrdSceneModel *
xrd_scene_model_new (VkDescriptorSetLayout *layout, GulkanClient *gulkan);

gboolean
xrd_scene_model_load (XrdSceneModel *self,
                      GxrContext    *context,
                      const char    *model_name);

VkSampler
xrd_scene_model_get_sampler (XrdSceneModel *self);

GulkanVertexBuffer*
xrd_scene_model_get_vbo (XrdSceneModel *self);

GulkanTexture*
xrd_scene_model_get_texture (XrdSceneModel *self);

void
xrd_scene_model_render (XrdSceneModel     *self,
                        GxrEye             eye,
                        VkPipeline         pipeline,
                        VkCommandBuffer    cmd_buffer,
                        VkPipelineLayout   pipeline_layout,
                        graphene_matrix_t *transformation,
                        graphene_matrix_t *vp);

G_END_DECLS

#endif /* XRD_SCENE_MODEL_H_ */
