/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_WINDOW_PRIVATE_H_
#define XRD_SCENE_WINDOW_PRIVATE_H_

#include "xrd-scene-window.h"

G_BEGIN_DECLS

gboolean
xrd_scene_window_initialize (XrdSceneWindow        *self,
                             GulkanClient          *gulkan,
                             VkDescriptorSetLayout *layout,
                             VkBuffer               lights);

XrdSceneWindow *
xrd_scene_window_new_from_meters (const gchar           *title,
                                  float                  width,
                                  float                  height,
                                  float                  ppm,
                                  GulkanClient          *gulkan,
                                  VkDescriptorSetLayout *layout,
                                  VkBuffer               lights);

XrdSceneWindow *
xrd_scene_window_new_from_data (XrdWindowData         *data,
                                GulkanClient          *gulkan,
                                VkDescriptorSetLayout *layout,
                                VkBuffer               lights);

XrdSceneWindow *
xrd_scene_window_new_from_pixels (const gchar           *title,
                                  uint32_t               width,
                                  uint32_t               height,
                                  float                  ppm,
                                  GulkanClient          *gulkan,
                                  VkDescriptorSetLayout *layout,
                                  VkBuffer               lights);

XrdSceneWindow *
xrd_scene_window_new_from_native (const gchar           *title,
                                  gpointer               native,
                                  uint32_t               width_pixels,
                                  uint32_t               height_pixels,
                                  float                  ppm,
                                  GulkanClient          *gulkan,
                                  VkDescriptorSetLayout *layout,
                                  VkBuffer               lights);

G_END_DECLS

#endif /* XRD_SCENE_WINDOW_PRIVATE_H_ */
