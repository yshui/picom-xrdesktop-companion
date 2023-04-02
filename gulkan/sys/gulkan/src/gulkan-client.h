/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_CLIENT_H_
#define GULKAN_CLIENT_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>

#include <vulkan/vulkan.h>

#include "gulkan-instance.h"
#include "gulkan-device.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_CLIENT gulkan_client_get_type()
G_DECLARE_DERIVABLE_TYPE (GulkanClient, gulkan_client, GULKAN, CLIENT, GObject)

struct _GulkanClientClass
{
  GObjectClass parent_class;
};

GulkanClient * gulkan_client_new (void);

GulkanClient * gulkan_client_new_from_extensions (GSList *instance_ext_list,
                                                  GSList *device_ext_list);

VkPhysicalDevice
gulkan_client_get_physical_device_handle (GulkanClient *self);

VkDevice
gulkan_client_get_device_handle (GulkanClient *self);

GulkanDevice *
gulkan_client_get_device (GulkanClient *self);

VkInstance
gulkan_client_get_instance_handle (GulkanClient *self);

GulkanInstance *
gulkan_client_get_instance (GulkanClient *self);

GSList *
gulkan_client_get_external_memory_instance_extensions (void);

GSList *
gulkan_client_get_external_memory_device_extensions (void);

G_END_DECLS

#endif /* GULKAN_CLIENT_H_ */
