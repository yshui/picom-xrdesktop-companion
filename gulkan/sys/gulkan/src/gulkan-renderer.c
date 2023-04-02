/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-renderer.h"

#include <sys/time.h>

#include <vulkan/vulkan.h>

typedef struct _GulkanRendererPrivate
{
  GObject parent;

  GulkanClient *client;

  struct timeval start;

  VkExtent2D extent;

} GulkanRendererPrivate;


G_DEFINE_TYPE_WITH_PRIVATE (GulkanRenderer, gulkan_renderer, G_TYPE_OBJECT)

static void
gulkan_renderer_finalize (GObject *gobject);


static void
gulkan_renderer_class_init (GulkanRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gulkan_renderer_finalize;
}

static void
gulkan_renderer_init (GulkanRenderer *self)
{
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  gettimeofday (&priv->start, NULL);
}

static void
gulkan_renderer_finalize (GObject *gobject)
{
  GulkanRenderer *self = GULKAN_RENDERER (gobject);
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  if (priv->client)
    g_object_unref (priv->client);
  G_OBJECT_CLASS (gulkan_renderer_parent_class)->finalize (gobject);
}

static gboolean
_load_resource (const gchar* path, GBytes **res)
{
  GError *error = NULL;
  *res = g_resources_lookup_data (path, G_RESOURCE_LOOKUP_FLAGS_NONE, &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }

  return TRUE;
}

gboolean
gulkan_renderer_create_shader_module (GulkanRenderer *self,
                                      const gchar* resource_name,
                                      VkShaderModule *module)
{
  GBytes *bytes;
  if (!_load_resource (resource_name, &bytes))
    return FALSE;

  gsize size = 0;
  const uint32_t *buffer = g_bytes_get_data (bytes, &size);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = buffer,
  };

  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  VkDevice device = gulkan_client_get_device_handle (priv->client);

  VkResult res = vkCreateShaderModule (device, &info, NULL, module);
  vk_check_error ("vkCreateShaderModule", res, FALSE);

  g_bytes_unref (bytes);

  return TRUE;
}

GulkanClient *
gulkan_renderer_get_client (GulkanRenderer *self)
{
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  return priv->client;
}

void
gulkan_renderer_set_client (GulkanRenderer *self, GulkanClient *client)
{
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  if (priv->client != client)
    priv->client = g_object_ref (client);
}

VkExtent2D
gulkan_renderer_get_extent (GulkanRenderer *self)
{
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  return priv->extent;
}

void
gulkan_renderer_set_extent (GulkanRenderer *self, VkExtent2D extent)
{
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);
  priv->extent = extent;
}

float
gulkan_renderer_get_aspect (GulkanRenderer *self)
{
  VkExtent2D extent = gulkan_renderer_get_extent (self);
  return (float) extent.width / (float) extent.height;
}

static int64_t
_timeval_to_msec (struct timeval *tv)
{
  return tv->tv_sec * 1000 + tv->tv_usec / 1000;
}

int64_t
gulkan_renderer_get_msec_since_start (GulkanRenderer *self)
{
  GulkanRendererPrivate *priv = gulkan_renderer_get_instance_private (self);

  struct timeval now;
  gettimeofday (&now, NULL);
  return _timeval_to_msec (&now) - _timeval_to_msec (&priv->start);
}

gboolean
gulkan_renderer_draw (GulkanRenderer *self)
{
  GulkanRendererClass *klass = GULKAN_RENDERER_GET_CLASS (self);
  if (klass->draw == NULL)
      return FALSE;
  return klass->draw (self);
}
