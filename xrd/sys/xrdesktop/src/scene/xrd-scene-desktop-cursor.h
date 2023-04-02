/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_DESKTOP_CURSOR_H_
#define XRD_SCENE_DESKTOP_CURSOR_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include "xrd-scene-window.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_DESKTOP_CURSOR xrd_scene_desktop_cursor_get_type()
G_DECLARE_FINAL_TYPE (XrdSceneDesktopCursor, xrd_scene_desktop_cursor,
                      XRD, SCENE_DESKTOP_CURSOR, XrdSceneWindow)


XrdSceneDesktopCursor *
xrd_scene_desktop_cursor_new (GulkanClient          *gulkan,
                              VkDescriptorSetLayout *layout,
                              VkBuffer               lights);

G_END_DECLS

#endif /* XRD_SCENE_DESKTOP_CURSOR_H_ */
