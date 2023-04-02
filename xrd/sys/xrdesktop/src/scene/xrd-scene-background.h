/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_BACKGROUND_H_
#define XRD_SCENE_BACKGROUND_H_

#include <glib-object.h>

#include <gxr.h>
#include <gulkan.h>

#include "xrd-scene-object.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_BACKGROUND xrd_scene_background_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneBackground, xrd_scene_background,
                      XRD, SCENE_BACKGROUND, XrdSceneObject)

XrdSceneBackground *
xrd_scene_background_new (GulkanClient          *gulkan,
                          VkDescriptorSetLayout *layout);

void
xrd_scene_background_render (XrdSceneBackground *self,
                             GxrEye              eye,
                             VkPipeline          pipeline,
                             VkPipelineLayout    pipeline_layout,
                             VkCommandBuffer     cmd_buffer,
                             graphene_matrix_t  *vp);

G_END_DECLS

#endif /* XRD_SCENE_BACKGROUND_H_ */
