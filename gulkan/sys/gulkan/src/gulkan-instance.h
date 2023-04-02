/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_INSTANCE_H_
#define GULKAN_INSTANCE_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>
#include <vulkan/vulkan.h>

G_BEGIN_DECLS

const gchar*
vk_result_string (VkResult code);

const gchar*
vk_format_string (VkFormat format);

gboolean
gulkan_has_error (VkResult     res,
                  const gchar *fun,
                  const gchar *file,
                  int          line);

#define vk_check_error(fun, res, ret) \
if (gulkan_has_error (res, fun, __FILE__, __LINE__)) \
  return ret

#define GULKAN_TYPE_INSTANCE gulkan_instance_get_type ()
G_DECLARE_FINAL_TYPE (GulkanInstance, gulkan_instance,
                      GULKAN, INSTANCE, GObject)

GulkanInstance *gulkan_instance_new (void);

gboolean
gulkan_instance_create (GulkanInstance *self,
                        GSList* requested_extensions);

VkInstance
gulkan_instance_get_handle (GulkanInstance *self);

G_END_DECLS

#endif /* GULKAN_INSTANCE_H_ */
