/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_UNIFORM_BUFFER_H_
#define GULKAN_UNIFORM_BUFFER_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <vulkan/vulkan.h>

#include <glib-object.h>

#include <gulkan-device.h>

#include <graphene.h>

G_BEGIN_DECLS

#define GULKAN_TYPE_UNIFORM_BUFFER gulkan_uniform_buffer_get_type()
G_DECLARE_FINAL_TYPE (GulkanUniformBuffer, gulkan_uniform_buffer,
                      GULKAN, UNIFORM_BUFFER, GObject)

GulkanUniformBuffer *
gulkan_uniform_buffer_new (GulkanDevice *device,
                           VkDeviceSize  size);

void
gulkan_uniform_buffer_update (GulkanUniformBuffer *self,
                              gpointer            *s);

VkBuffer
gulkan_uniform_buffer_get_handle (GulkanUniformBuffer *self);

G_END_DECLS

#endif /* GULKAN_UNIFORM_BUFFER_H_ */
