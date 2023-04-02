/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-texture.h"

#include <vulkan/vulkan.h>
#include "gulkan-buffer.h"
#include "gulkan-cmd-buffer.h"

struct _GulkanTexture
{
  GObjectClass parent;

  GulkanClient *client;

  VkImage image;
  VkDeviceMemory image_memory;
  VkImageView image_view;

  guint mip_levels;

  VkExtent2D extent;

  VkFormat format;
};

G_DEFINE_TYPE (GulkanTexture, gulkan_texture, G_TYPE_OBJECT)

typedef struct {
  uint32_t           levels;
  uint8_t           *buffer;
  VkDeviceSize       size;
  VkBufferImageCopy *buffer_image_copies;
} GulkanMipMap;

static GulkanMipMap
_generate_mipmaps (GdkPixbuf *pixbuf);

static void
gulkan_texture_init (GulkanTexture *self)
{
  self->image = VK_NULL_HANDLE;
  self->image_memory = VK_NULL_HANDLE;
  self->image_view = VK_NULL_HANDLE;
  self->format = VK_FORMAT_UNDEFINED;
  self->mip_levels = 1;
}

static void
_finalize (GObject *gobject)
{
  GulkanTexture *self = GULKAN_TEXTURE (gobject);
  VkDevice device = gulkan_client_get_device_handle (self->client);

  vkDestroyImageView (device, self->image_view, NULL);
  vkDestroyImage (device, self->image, NULL);
  vkFreeMemory (device, self->image_memory, NULL);

  g_object_unref (self->client);

  G_OBJECT_CLASS (gulkan_texture_parent_class)->finalize (gobject);
}

static void
gulkan_texture_class_init (GulkanTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

static gboolean
_upload_pixels (GulkanTexture           *self,
                guchar                  *pixels,
                gsize                    size,
                const VkBufferImageCopy *regions,
                VkImageLayout            layout)
{
  GulkanDevice *device = gulkan_client_get_device (self->client);
  GulkanQueue *queue = gulkan_device_get_transfer_queue (device);
  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  GMutex *mutex = gulkan_queue_get_pool_mutex (queue);

  g_mutex_lock (mutex);
  gboolean ret = gulkan_cmd_buffer_begin (cmd_buffer);
  g_mutex_unlock (mutex);

  if (!ret)
    return FALSE;

  GulkanBuffer *staging_buffer =
    gulkan_buffer_new (device, size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  if (!staging_buffer)
    return FALSE;

  if (!gulkan_buffer_upload (staging_buffer, pixels, size))
    return FALSE;

  gulkan_texture_record_transfer (self,
                                  gulkan_cmd_buffer_get_handle (cmd_buffer),
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  g_mutex_lock (mutex);
  vkCmdCopyBufferToImage (gulkan_cmd_buffer_get_handle (cmd_buffer),
                          gulkan_buffer_get_handle (staging_buffer),
                          self->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          self->mip_levels,
                          regions);
  g_mutex_unlock (mutex);

  gulkan_texture_record_transfer (self,
                                  gulkan_cmd_buffer_get_handle (cmd_buffer),
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  layout);

  if (!gulkan_queue_submit (queue, cmd_buffer))
    return FALSE;

  g_object_unref (staging_buffer);
  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

  return TRUE;
}

GulkanTexture *
gulkan_texture_new_from_pixbuf (GulkanClient   *client,
                                GdkPixbuf      *pixbuf,
                                VkFormat        format,
                                VkImageLayout   layout,
                                gboolean        create_mipmaps)
{
  VkExtent2D extent = {
    .width = (uint32_t) gdk_pixbuf_get_width (pixbuf),
    .height = (uint32_t) gdk_pixbuf_get_height (pixbuf)
  };

  GulkanTexture *self;

  if (create_mipmaps)
    {
      GulkanMipMap mipmap = _generate_mipmaps (pixbuf);

      self = gulkan_texture_new_mip_levels (client, extent,
                                            mipmap.levels, format);

      if(!_upload_pixels (self, mipmap.buffer, mipmap.size,
                          mipmap.buffer_image_copies, layout))
        {
          g_printerr ("ERROR: Could not upload pixels.\n");
          g_object_unref (self);
          self = NULL;
        }

      g_free (mipmap.buffer);
      g_free (mipmap.buffer_image_copies);
    }
  else
    {
      gsize size = gdk_pixbuf_get_byte_length (pixbuf);
      guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
      self = gulkan_texture_new (client, extent, format);

      if (!gulkan_texture_upload_pixels (self, pixels, size, layout))
        {
          g_printerr ("ERROR: Could not upload pixels.\n");
          g_object_unref (self);
          self = NULL;
        }
    }

  return self;
}

GulkanTexture *
gulkan_texture_new_from_cairo_surface (GulkanClient    *client,
                                       cairo_surface_t *surface,
                                       VkFormat         format,
                                       VkImageLayout    layout)
{
  VkExtent2D extent = {
    .width = (uint32_t) cairo_image_surface_get_width (surface),
    .height = (uint32_t) cairo_image_surface_get_height (surface)
  };

  guint stride = (guint)cairo_image_surface_get_stride (surface);

  guint size = stride * extent.height;

  guchar *pixels = cairo_image_surface_get_data (surface);

  GulkanTexture *self = gulkan_texture_new (client, extent, format);

  if (!gulkan_texture_upload_pixels (self, pixels, size, layout))
    {
      g_printerr ("ERROR: Could not upload pixels.\n");
      g_object_unref (self);
      self = NULL;
    }

  return self;
}

GulkanTexture *
gulkan_texture_new (GulkanClient *client,
                    VkExtent2D    extent,
                    VkFormat      format)
{
  return gulkan_texture_new_mip_levels (client, extent, 1, format);
}

GulkanTexture *
gulkan_texture_new_mip_levels (GulkanClient *client,
                               VkExtent2D    extent,
                               guint         mip_levels,
                               VkFormat      format)
{
  GulkanTexture *self = (GulkanTexture*) g_object_new (GULKAN_TYPE_TEXTURE, 0);

  self->extent = extent;
  self->client = g_object_ref (client);
  self->format = format;
  self->mip_levels = mip_levels;

  VkDevice vk_device = gulkan_client_get_device_handle (client);

  /* TODO: Check with vkGetPhysicalDeviceFormatProperties */
  VkImageTiling tiling;
  switch (format)
    {
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8_UNORM:
      tiling = VK_IMAGE_TILING_LINEAR;
      break;
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_UNORM:
      tiling = VK_IMAGE_TILING_OPTIMAL;
      break;
    default:
      g_printerr ("Warning: No tiling for format %s (%d) specified.\n",
                  vk_format_string(format), format);
      tiling = VK_IMAGE_TILING_OPTIMAL;
    }

  VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent.width = extent.width,
    .extent.height = extent.height,
    .extent.depth = 1,
    .mipLevels = mip_levels,
    .arrayLayers = 1,
    .format = format,
    .tiling = tiling,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .flags = 0
  };
  VkResult res;
  res = vkCreateImage (vk_device, &image_info, NULL, &self->image);
  vk_check_error ("vkCreateImage", res, NULL);

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements (vk_device, self->image,
                                &memory_requirements);

  VkMemoryAllocateInfo memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size
  };
  gulkan_device_memory_type_from_properties (
    gulkan_client_get_device (client),
    memory_requirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &memory_info.memoryTypeIndex);
  res = vkAllocateMemory (vk_device, &memory_info,
                          NULL, &self->image_memory);
  vk_check_error ("vkAllocateMemory", res, NULL);
  res = vkBindImageMemory (vk_device, self->image, self->image_memory, 0);
  vk_check_error ("vkBindImageMemory", res, NULL);

  VkImageViewCreateInfo image_view_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = self->image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_info.format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = image_info.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  res = vkCreateImageView (vk_device, &image_view_info,
                           NULL, &self->image_view);
  vk_check_error ("vkCreateImageView", res, NULL);

  return self;
}

static GulkanMipMap
_generate_mipmaps (GdkPixbuf *pixbuf)
{
  int width = gdk_pixbuf_get_width (pixbuf);
  int height = gdk_pixbuf_get_height (pixbuf);

  GulkanMipMap mipmap = {
    .levels = 1,
    .size = sizeof (uint8_t) * (guint) width * (guint) height * 4 * 2,
  };

  mipmap.buffer = g_malloc (mipmap.size);

  /* Test how many levels we will be generating */
  int test_width = width;
  int test_height = height;
  while (test_width > 1 && test_height > 1)
    {
      test_width /= 2;
      test_height /= 2;
      mipmap.levels++;
    }

  mipmap.buffer_image_copies =
    g_malloc (sizeof(VkBufferImageCopy) * mipmap.levels);

  /* Original size */
  VkBufferImageCopy buffer_image_copy = {
    .imageSubresource = {
      .baseArrayLayer = 0,
      .layerCount = 1,
      .mipLevel = 0,
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    },
    .imageExtent = {
      .width = (guint) width,
      .height = (guint) height,
      .depth = 1
    },
  };

  memcpy (&mipmap.buffer_image_copies[0], &buffer_image_copy,
          sizeof(VkBufferImageCopy));

  uint8_t *current = mipmap.buffer;
  gsize original_size = gdk_pixbuf_get_byte_length (pixbuf);
  memcpy (current, gdk_pixbuf_get_pixels (pixbuf), original_size);
  current += original_size;

  /* MIP levels */
  uint32_t level = 1;
  int mip_width = width;
  int mip_height = height;
  GdkPixbuf *last_pixbuf = pixbuf;
  while (mip_width > 1 && mip_height > 1)
    {
      mip_width /= 2;
      mip_height /= 2;

      GdkPixbuf *level_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                               TRUE, 8,
                                               MAX (mip_width, 1),
                                               MAX (mip_height, 1));

      gdk_pixbuf_scale (last_pixbuf, level_pixbuf, 0, 0,
                        gdk_pixbuf_get_width (level_pixbuf),
                        gdk_pixbuf_get_height (level_pixbuf),
                        0.0, 0.0, 0.5, 0.5, GDK_INTERP_BILINEAR);

      if (last_pixbuf != pixbuf)
        g_object_unref (last_pixbuf);
      last_pixbuf = level_pixbuf;

      /* Copy pixels */
      memcpy (current,
              gdk_pixbuf_get_pixels (level_pixbuf),
              gdk_pixbuf_get_byte_length (level_pixbuf));

      /* Copy VkBufferImageCopy */
      buffer_image_copy.bufferOffset = (VkDeviceSize) (current - mipmap.buffer);
      buffer_image_copy.imageSubresource.mipLevel = level;
      buffer_image_copy.imageExtent.width =
        (guint) gdk_pixbuf_get_width (level_pixbuf);
      buffer_image_copy.imageExtent.height =
        (guint) gdk_pixbuf_get_height (level_pixbuf);
      memcpy (&mipmap.buffer_image_copies[level], &buffer_image_copy,
              sizeof(VkBufferImageCopy));

      level++;
      current += gdk_pixbuf_get_byte_length (level_pixbuf);
    }

  g_object_unref (last_pixbuf);

  return mipmap;
}

gboolean
gulkan_texture_upload_pixels (GulkanTexture  *self,
                              guchar         *pixels,
                              gsize           size,
                              VkImageLayout   layout)
{
  if (self->mip_levels != 1)
    {
      g_warning ("Trying to upload one mip level to multi level texture.\n");
      return FALSE;
    }

  VkBufferImageCopy buffer_image_copy = {
    .imageSubresource = {
      .baseArrayLayer = 0,
      .layerCount = 1,
      .mipLevel = 0,
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    },
    .imageExtent = {
      .width = self->extent.width,
      .height = self->extent.height,
      .depth = 1,
    }
  };

  return _upload_pixels (self, pixels, size, &buffer_image_copy, layout);
}

gboolean
gulkan_texture_upload_pixbuf (GulkanTexture *self,
                              GdkPixbuf     *pixbuf,
                              VkImageLayout  layout)
{
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
  gsize size = gdk_pixbuf_get_byte_length (pixbuf);

  return gulkan_texture_upload_pixels (self, pixels, size, layout);
}

gboolean
gulkan_texture_upload_cairo_surface (GulkanTexture   *self,
                                     cairo_surface_t *surface,
                                     VkImageLayout    layout)
{
  guchar *pixels = cairo_image_surface_get_data (surface);
  gsize size = (gsize) cairo_image_surface_get_stride (surface) *
               (gsize) cairo_image_surface_get_height (surface);

  return gulkan_texture_upload_pixels (self, pixels, size, layout);
}


GulkanTexture *
gulkan_texture_new_from_dmabuf (GulkanClient *client,
                                int           fd,
                                VkExtent2D    extent,
                                VkFormat      format)
{
  GulkanTexture *self = (GulkanTexture*) g_object_new (GULKAN_TYPE_TEXTURE, 0);
  VkDevice vk_device = gulkan_client_get_device_handle (client);

  self->extent = extent;
  self->client = g_object_ref (client);
  self->format = format;

  VkExternalMemoryImageCreateInfoKHR external_memory_image_create_info = {
    .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
    .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR |
                   VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
  };

  VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = &external_memory_image_create_info,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent = {
      .width = extent.width,
      .height = extent.height,
      .depth = 1,
    },
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_LINEAR,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    /* DMA buffer only allowed to import as UNDEFINED */
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
  };

  VkResult res;
  res = vkCreateImage (vk_device, &image_info, NULL, &self->image);
  vk_check_error ("vkCreateImage", res, NULL);

  VkMemoryDedicatedAllocateInfoKHR dedicated_memory_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
    .pNext = NULL,
    .image = self->image,
    .buffer = VK_NULL_HANDLE
  };

  VkImportMemoryFdInfoKHR import_memory_info = {
    .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
    .pNext = &dedicated_memory_info,
    .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
    .fd = fd
  };

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements (vk_device, self->image,
                                &memory_requirements);

  VkMemoryAllocateInfo memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = &import_memory_info,
    .allocationSize = memory_requirements.size
  };

  gulkan_device_memory_type_from_properties (
    gulkan_client_get_device (client),
    memory_requirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    &memory_info.memoryTypeIndex);

  res = vkAllocateMemory (vk_device, &memory_info,
                          NULL, &self->image_memory);
  vk_check_error ("vkAllocateMemory", res, NULL);

  res = vkBindImageMemory (vk_device, self->image, self->image_memory, 0);
  vk_check_error ("vkBindImageMemory", res, NULL);

  VkImageViewCreateInfo image_view_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .flags = 0,
    .image = self->image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_info.format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  res = vkCreateImageView (vk_device, &image_view_info,
                           NULL, &self->image_view);
  vk_check_error ("vkCreateImageView", res, NULL);

  return self;
}

/**
 * gulkan_texture_new_export_fd:
 * @client: a #GulkanClient
 * @extent: Extent in pixels
 * @format: VkFormat of the texture
 * @layout: VkImageLayout of the texture
 * @size: Return value of allocated size
 * @fd: Return value for allocated fd
 *
 * Allocates a #GulkanTexture and exports it via external memory to an fd and
 * provides the size of the external memory.
 *
 * based on code from
 * https://github.com/lostgoat/ogl-samples/blob/master/tests/gl-450-culling.cpp
 * https://gitlab.com/beVR_nz/VulkanIPC_Demo/
 *
 * Returns: the initialized #GulkanTexture
 */
GulkanTexture *
gulkan_texture_new_export_fd (GulkanClient *client,
                              VkExtent2D    extent,
                              VkFormat      format,
                              VkImageLayout layout,
                              gsize        *size,
                              int          *fd)
{
  GulkanTexture *self = (GulkanTexture*) g_object_new (GULKAN_TYPE_TEXTURE, 0);
  VkDevice vk_device = gulkan_client_get_device_handle (client);

  self->extent = extent;
  self->client = g_object_ref (client);
  self->format = format;

  /* we can also export the memory of the image without using this struct but
   * the spec makes it sound like we should use it */
  VkExternalMemoryImageCreateInfo external_memory_image_create_info = {
    .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
    .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
                   VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
  };

  VkImageCreateInfo image_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = &external_memory_image_create_info,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent = {
      .width = extent.width,
      .height = extent.height,
      .depth = 1,
    },
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
             VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
  };

  VkResult res;
  res = vkCreateImage (vk_device, &image_info, NULL, &self->image);
  vk_check_error ("vkCreateImage", res, NULL);

  VkMemoryDedicatedAllocateInfoKHR dedicated_memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
    .pNext = NULL,
    .image = self->image,
    .buffer = VK_NULL_HANDLE
  };

  VkMemoryRequirements memory_reqs;
  vkGetImageMemoryRequirements (vk_device, self->image,
                                &memory_reqs);

  *size = memory_reqs.size;

  VkMemoryAllocateInfo memory_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = &dedicated_memory_info,
    .allocationSize = memory_reqs.size,
    /* filled in later */
    .memoryTypeIndex = 0
  };

  VkMemoryPropertyFlags full_memory_property_flags =
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  GulkanDevice *device = gulkan_client_get_device (client);

  gboolean full_flags_available =
    gulkan_device_memory_type_from_properties (device,
                                               memory_reqs.memoryTypeBits,
                                               full_memory_property_flags,
                                              &memory_info.memoryTypeIndex);

  if (!full_flags_available)
    {
      VkMemoryPropertyFlags fallback_memory_property_flags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

      if (!gulkan_device_memory_type_from_properties (device,
                                                      memory_reqs.memoryTypeBits,
                                                      fallback_memory_property_flags,
                                                     &memory_info.memoryTypeIndex))
        {
          g_printerr ("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT"
                      " memory flags not available.\n");
          return NULL;
        }
    }

  res = vkAllocateMemory (vk_device, &memory_info,
                          NULL, &self->image_memory);
  vk_check_error ("vkAllocateMemory", res, NULL);

  res = vkBindImageMemory (vk_device, self->image, self->image_memory, 0);
  vk_check_error ("vkBindImageMemory", res, NULL);


  VkImageViewCreateInfo image_view_info =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .flags = 0,
    .image = self->image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_info.format,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
  res = vkCreateImageView (vk_device, &image_view_info,
                           NULL, &self->image_view);
  vk_check_error ("vkCreateImageView", res, NULL);

  if (!gulkan_device_get_memory_fd (device, self->image_memory, fd))
    {
      g_printerr ("Could not get file descriptor for memory!\n");
      g_object_unref (self);
      return NULL;
    }

  GulkanQueue *queue = gulkan_device_get_transfer_queue (device);
  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  GMutex *mutex = gulkan_queue_get_pool_mutex (queue);

  g_mutex_lock (mutex);
  gboolean ret = gulkan_cmd_buffer_begin (cmd_buffer);
  g_mutex_unlock (mutex);
  if (!ret)
    return FALSE;

  gulkan_texture_record_transfer (self,
                                  gulkan_cmd_buffer_get_handle (cmd_buffer),
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  gulkan_texture_record_transfer (self,
                                  gulkan_cmd_buffer_get_handle (cmd_buffer),
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  layout);

  if (!gulkan_queue_submit (queue, cmd_buffer))
    return FALSE;

  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

  return self;
}

static VkAccessFlags
_get_access_flags (VkImageLayout layout)
{
  switch (layout)
    {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_ACCESS_HOST_WRITE_BIT;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
      default:
        g_warning ("Unhandled access mask case for layout %d.\n", layout);
    }
  return 0;
}

void
gulkan_texture_record_transfer (GulkanTexture       *self,
                                VkCommandBuffer      cmd_buffer,
                                VkImageLayout        src_layout,
                                VkImageLayout        dst_layout)
{
  gulkan_texture_record_transfer_full (self,
                                       cmd_buffer,
                                       _get_access_flags (src_layout),
                                       _get_access_flags (dst_layout),
                                       src_layout, dst_layout,
                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

void
gulkan_texture_record_transfer_full (GulkanTexture       *self,
                                     VkCommandBuffer      cmd_buffer,
                                     VkAccessFlags        src_access_mask,
                                     VkAccessFlags        dst_access_mask,
                                     VkImageLayout        src_layout,
                                     VkImageLayout        dst_layout,
                                     VkPipelineStageFlags src_stage_mask,
                                     VkPipelineStageFlags dst_stage_mask)
{
  GulkanDevice *device = gulkan_client_get_device (self->client);
  GulkanQueue *gulkan_queue = gulkan_device_get_transfer_queue (device);
  uint32_t queue_index = gulkan_queue_get_family_index (gulkan_queue);
  GMutex *mutex = gulkan_queue_get_pool_mutex (gulkan_queue);

  VkImageMemoryBarrier image_memory_barrier =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = src_access_mask,
    .dstAccessMask = dst_access_mask,
    .oldLayout = src_layout,
    .newLayout = dst_layout,
    .image = self->image,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = self->mip_levels,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
    .srcQueueFamilyIndex = queue_index,
    .dstQueueFamilyIndex = queue_index
  };

  g_mutex_lock (mutex);
  vkCmdPipelineBarrier (cmd_buffer,
                        src_stage_mask,
                        dst_stage_mask,
                        0, 0, NULL, 0, NULL, 1,
                        &image_memory_barrier);
  g_mutex_unlock (mutex);
}

gboolean
gulkan_texture_transfer_layout_full (GulkanTexture       *self,
                                     VkAccessFlags        src_access_mask,
                                     VkAccessFlags        dst_access_mask,
                                     VkImageLayout        src_layout,
                                     VkImageLayout        dst_layout,
                                     VkPipelineStageFlags src_stage_mask,
                                     VkPipelineStageFlags dst_stage_mask)
{
  GulkanDevice *device = gulkan_client_get_device (self->client);
  GulkanQueue *queue = gulkan_device_get_transfer_queue (device);
  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  GMutex *mutex = gulkan_queue_get_pool_mutex (queue);
  g_mutex_lock (mutex);
  gboolean ret = gulkan_cmd_buffer_begin (cmd_buffer);
  g_mutex_unlock (mutex);
  if (!ret)
    return FALSE;

  gulkan_texture_record_transfer_full (self,
                                       gulkan_cmd_buffer_get_handle (cmd_buffer),
                                       src_access_mask, dst_access_mask,
                                       src_layout, dst_layout,
                                       src_stage_mask, dst_stage_mask);

  if (!gulkan_queue_submit (queue, cmd_buffer))
    return FALSE;

  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

  return TRUE;
}

gboolean
gulkan_texture_transfer_layout (GulkanTexture *self,
                                VkImageLayout  src_layout,
                                VkImageLayout  dst_layout)
{
  GulkanDevice *device = gulkan_client_get_device (self->client);
  GulkanQueue *queue = gulkan_device_get_transfer_queue (device);
  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  GMutex *mutex = gulkan_queue_get_pool_mutex (queue);
  g_mutex_lock (mutex);
  gboolean ret = gulkan_cmd_buffer_begin (cmd_buffer);
  g_mutex_unlock (mutex);

  if (!ret)
    return FALSE;

  gulkan_texture_record_transfer (self,
                                  gulkan_cmd_buffer_get_handle (cmd_buffer),
                                  src_layout, dst_layout);

  if (!gulkan_queue_submit (queue, cmd_buffer))
    return FALSE;

  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

  return TRUE;
}

VkImageView
gulkan_texture_get_image_view (GulkanTexture *self)
{
  return self->image_view;
}

VkImage
gulkan_texture_get_image (GulkanTexture *self)
{
  return self->image;
}

VkExtent2D
gulkan_texture_get_extent (GulkanTexture *self)
{
  return self->extent;
}

VkFormat
gulkan_texture_get_format (GulkanTexture *self)
{
  return self->format;
}

guint
gulkan_texture_get_mip_levels (GulkanTexture *self)
{
  return self->mip_levels;
}
