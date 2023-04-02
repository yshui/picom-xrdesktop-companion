/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-client.h"

typedef struct _GulkanClientPrivate
{
  GObject object;

  GulkanInstance *instance;
  GulkanDevice *device;
} GulkanClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GulkanClient, gulkan_client, G_TYPE_OBJECT)

static void
gulkan_client_finalize (GObject *gobject);

static void
gulkan_client_class_init (GulkanClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gulkan_client_finalize;
}

static void
gulkan_client_init (GulkanClient *self)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  priv->instance = gulkan_instance_new ();
  priv->device = gulkan_device_new ();

}

static gboolean
_init_vulkan_full (GulkanClient    *self,
                   GSList          *instance_extensions,
                   GSList          *device_extensions,
                   VkPhysicalDevice physical_device);

static GulkanClient *
_client_new (GSList *instance_ext_list, GSList *device_ext_list)
{
  GulkanClient *self = (GulkanClient*) g_object_new (GULKAN_TYPE_CLIENT, 0);
  if (!_init_vulkan_full (self, instance_ext_list, device_ext_list,
                          VK_NULL_HANDLE))
    {
      g_object_unref (self);
      return NULL;
    }
  return self;
}

GulkanClient *
gulkan_client_new (void)
{
  return _client_new (NULL, NULL);
}

GulkanClient *
gulkan_client_new_from_extensions (GSList *instance_ext_list,
                                   GSList *device_ext_list)
{
  return _client_new (instance_ext_list, device_ext_list);
}

static void
gulkan_client_finalize (GObject *gobject)
{
  GulkanClient *self = GULKAN_CLIENT (gobject);

  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  g_object_unref (priv->device);
  g_object_unref (priv->instance);
}

static gboolean
_init_vulkan_full (GulkanClient    *self,
                   GSList          *instance_extensions,
                   GSList          *device_extensions,
                   VkPhysicalDevice physical_device)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  if (!gulkan_instance_create (priv->instance,
                               instance_extensions))
    {
      g_printerr ("Failed to create instance.\n");
      return FALSE;
    }

  if (!gulkan_device_create (priv->device, priv->instance,
                             physical_device, device_extensions))
    {
      g_printerr ("Failed to create device.\n");
      return FALSE;
    }

  return TRUE;
}

VkPhysicalDevice
gulkan_client_get_physical_device_handle (GulkanClient *self)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  return gulkan_device_get_physical_handle (priv->device);
}

VkDevice
gulkan_client_get_device_handle (GulkanClient *self)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  return gulkan_device_get_handle (priv->device);
}

GulkanDevice *
gulkan_client_get_device (GulkanClient *self)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  return priv->device;
}

VkInstance
gulkan_client_get_instance_handle (GulkanClient *self)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  return gulkan_instance_get_handle (priv->instance);
}

GulkanInstance *
gulkan_client_get_instance (GulkanClient *self)
{
  GulkanClientPrivate *priv = gulkan_client_get_instance_private (self);
  return priv->instance;
}

GSList *
gulkan_client_get_external_memory_instance_extensions (void)
{
  const gchar *instance_extensions[] = {
    VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME
  };

  GSList *instance_ext_list = NULL;
  for (uint64_t i = 0; i < G_N_ELEMENTS (instance_extensions); i++)
    {
      char *instance_ext = g_strdup (instance_extensions[i]);
      instance_ext_list = g_slist_append (instance_ext_list, instance_ext);
    }

  return instance_ext_list;
}

GSList *
gulkan_client_get_external_memory_device_extensions (void)
{
  const gchar *device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
    VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
  };

  GSList *device_ext_list = NULL;
  for (uint64_t i = 0; i < G_N_ELEMENTS (device_extensions); i++)
    {
      char *device_ext = g_strdup (device_extensions[i]);
      device_ext_list = g_slist_append (device_ext_list, device_ext);
    }

  return device_ext_list;
}
