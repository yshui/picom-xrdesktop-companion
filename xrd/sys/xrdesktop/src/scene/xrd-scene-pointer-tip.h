/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_POINTER_TIP_H_
#define XRD_SCENE_POINTER_TIP_H_

#include <glib-object.h>
#include "gulkan.h"
#include "gxr.h"
#include "xrd-scene-object.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_POINTER_TIP xrd_scene_pointer_tip_get_type()
G_DECLARE_FINAL_TYPE (XrdScenePointerTip, xrd_scene_pointer_tip,
                      XRD, SCENE_POINTER_TIP, XrdSceneObject)

XrdScenePointerTip *
xrd_scene_pointer_tip_new (GulkanClient *gulkan,
                           VkDescriptorSetLayout *layout,
                           VkBuffer lights);

void
xrd_scene_pointer_tip_render (XrdScenePointerTip *self,
                              GxrEye              eye,
                              VkPipeline          pipeline,
                              VkPipelineLayout    pipeline_layout,
                              VkCommandBuffer     cmd_buffer,
                              graphene_matrix_t  *vp);

G_END_DECLS

#endif /* XRD_SCENE_POINTER_TIP_H_ */
