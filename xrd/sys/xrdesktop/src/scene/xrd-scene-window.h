/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_WINDOW_H_
#define XRD_SCENE_WINDOW_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include <gxr.h>

#include <gulkan.h>

#include "xrd-scene-object.h"
#include "xrd-window.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_WINDOW xrd_scene_window_get_type()
G_DECLARE_DERIVABLE_TYPE (XrdSceneWindow, xrd_scene_window,
                          XRD, SCENE_WINDOW, XrdSceneObject)

/**
 * XrdSceneWindowClass:
 * @parent: The object class structure needs to be the first
 *   element in the widget class structure in order for the class mechanism
 *   to work correctly. This allows a XrdSceneWindowClass pointer to be cast to
 *   a XrdSceneObjectClass pointer.
 */
struct _XrdSceneWindowClass
{
  XrdSceneObjectClass parent;
};

void
xrd_scene_window_render (XrdSceneWindow    *self,
                         GxrEye             eye,
                         VkPipeline         pipeline,
                         VkPipelineLayout   pipeline_layout,
                         VkCommandBuffer    cmd_buffer,
                         graphene_matrix_t *view,
                         graphene_matrix_t *projection,
                         gboolean           shaded);

void
xrd_scene_window_set_width_meters (XrdSceneWindow *self,
                                   float           width_meters);

void
xrd_scene_window_set_color (XrdSceneWindow        *self,
                            const graphene_vec3_t *color);

void
xrd_scene_window_update_descriptors (XrdSceneWindow *self);

G_END_DECLS

#endif /* XRD_SCENE_WINDOW_H_ */
