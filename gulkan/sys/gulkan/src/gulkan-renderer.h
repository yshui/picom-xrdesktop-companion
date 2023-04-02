/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_RENDERER_H_
#define GULKAN_RENDERER_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include "gulkan-client.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_RENDERER gulkan_renderer_get_type()
G_DECLARE_DERIVABLE_TYPE (GulkanRenderer, gulkan_renderer,
                          GULKAN, RENDERER, GObject)

struct _GulkanRendererClass
{
  GObjectClass parent;

  gboolean
  (*draw) (GulkanRenderer *self);
};

gboolean
gulkan_renderer_create_shader_module (GulkanRenderer *self,
                                      const gchar* resource_name,
                                      VkShaderModule *module);

GulkanClient *
gulkan_renderer_get_client (GulkanRenderer *self);

VkExtent2D
gulkan_renderer_get_extent (GulkanRenderer *self);

void
gulkan_renderer_set_client (GulkanRenderer *self, GulkanClient *client);

void
gulkan_renderer_set_extent (GulkanRenderer *self, VkExtent2D extent);

float
gulkan_renderer_get_aspect (GulkanRenderer *self);

int64_t
gulkan_renderer_get_msec_since_start (GulkanRenderer *self);

gboolean
gulkan_renderer_draw (GulkanRenderer *self);

G_END_DECLS

#endif /* GULKAN_RENDERER_H_ */
