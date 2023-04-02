/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_POINTER_H_
#define XRD_SCENE_POINTER_H_

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

#include "xrd-scene-object.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_POINTER xrd_scene_pointer_get_type()
G_DECLARE_FINAL_TYPE (XrdScenePointer, xrd_scene_pointer,
                      XRD, SCENE_POINTER, XrdSceneObject)

XrdScenePointer *
xrd_scene_pointer_new (GulkanClient          *gulkan,
                       VkDescriptorSetLayout *layout);

void
xrd_scene_pointer_render (XrdScenePointer   *self,
                          GxrEye             eye,
                          VkPipeline         pipeline,
                          VkPipelineLayout   pipeline_layout,
                          VkCommandBuffer    cmd_buffer,
                          graphene_matrix_t *vp);

G_END_DECLS

#endif /* XRD_SCENE_POINTER_H_ */
