/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-frame-buffer.h"
#include "gulkan-cmd-buffer.h"

struct _GulkanFrameBuffer
{
  GObject parent;

  GulkanDevice *device;

  /*
  GulkanTexture *color_texture;
  GulkanTexture *depth_stencil_texture;
  */
  VkImage        color_image;
  VkDeviceMemory color_memory;
  VkImageView    color_image_view;
  VkImage        depth_stencil_image;
  VkDeviceMemory depth_stencil_memory;
  VkImageView    depth_stencil_image_view;

  VkFramebuffer  framebuffer;

  VkExtent2D extent;

  gboolean use_depth;
};

G_DEFINE_TYPE (GulkanFrameBuffer, gulkan_frame_buffer, G_TYPE_OBJECT)

static void
gulkan_frame_buffer_finalize (GObject *gobject);

static void
gulkan_frame_buffer_class_init (GulkanFrameBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gulkan_frame_buffer_finalize;
}

static void
gulkan_frame_buffer_init (GulkanFrameBuffer *self)
{
  self->color_image = VK_NULL_HANDLE;
  self->color_memory = VK_NULL_HANDLE;

  self->depth_stencil_image = VK_NULL_HANDLE;
  self->depth_stencil_memory = VK_NULL_HANDLE;
  self->depth_stencil_image_view = VK_NULL_HANDLE;
}

static void
gulkan_frame_buffer_finalize (GObject *gobject)
{
  GulkanFrameBuffer *self = GULKAN_FRAME_BUFFER (gobject);
  if (self->color_image_view == VK_NULL_HANDLE)
    return;

  VkDevice device = gulkan_device_get_handle (self->device);

  vkDestroyImageView (device, self->color_image_view, NULL);
  if (self->color_image != VK_NULL_HANDLE)
    vkDestroyImage (device, self->color_image, NULL);
  if (self->color_memory != VK_NULL_HANDLE)
    vkFreeMemory (device, self->color_memory, NULL);

  if (self->depth_stencil_image_view != VK_NULL_HANDLE)
    vkDestroyImageView (device, self->depth_stencil_image_view, NULL);
  if (self->depth_stencil_image != VK_NULL_HANDLE)
    vkDestroyImage (device, self->depth_stencil_image, NULL);
  if (self->depth_stencil_memory != VK_NULL_HANDLE)
    vkFreeMemory (device, self->depth_stencil_memory, NULL);

  vkDestroyFramebuffer (device, self->framebuffer, NULL);
}

static gboolean
_create_image_view (GulkanFrameBuffer     *self,
                    VkImage                image,
                    VkFormat               format,
                    VkImageAspectFlagBits  aspect_flag,
                    VkImageView           *out_image_view)
{
  VkImageViewCreateInfo image_view_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .flags = 0,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY
    },
    .subresourceRange = {
      .aspectMask = aspect_flag,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };

  VkResult res = vkCreateImageView (gulkan_device_get_handle (self->device),
                                   &image_view_info, NULL, out_image_view);
  vk_check_error ("vkCreateImageView", res, FALSE);

  return TRUE;
}

static VkAccessFlags
_get_access_flags (VkImageLayout layout)
{
  switch (layout)
  {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      return 0;
    case VK_IMAGE_LAYOUT_GENERAL:
      return VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
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

static void
_record_transfer_full (VkImage              image,
                       GulkanDevice        *device,
                       uint32_t             mip_levels,
                       VkCommandBuffer      cmd_buffer,
                       VkAccessFlags        src_access_mask,
                       VkAccessFlags        dst_access_mask,
                       gboolean             is_depth_format,
                       VkImageLayout        src_layout,
                       VkImageLayout        dst_layout,
                       VkPipelineStageFlags src_stage_mask,
                       VkPipelineStageFlags dst_stage_mask)
{
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
    .image = image,
    .subresourceRange = {
      .aspectMask = is_depth_format ?
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = mip_levels,
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

static gboolean
_transfer_layout (VkImage       image,
                  GulkanDevice *device,
                  uint32_t      mip_levels,
                  gboolean      is_depth_format,
                  VkImageLayout src_layout,
                  VkImageLayout dst_layout)
{
  GulkanQueue *queue = gulkan_device_get_transfer_queue (device);
  GulkanCmdBuffer *cmd_buffer = gulkan_queue_request_cmd_buffer (queue);
  GMutex *mutex = gulkan_queue_get_pool_mutex (queue);

  g_mutex_lock (mutex);
  gboolean ret = gulkan_cmd_buffer_begin (cmd_buffer);
  g_mutex_unlock (mutex);
  if (!ret)
    return FALSE;

  _record_transfer_full (image, device, mip_levels,
                         gulkan_cmd_buffer_get_handle (cmd_buffer),
                         _get_access_flags (src_layout),
                         _get_access_flags (dst_layout),
                         is_depth_format,
                         src_layout, dst_layout,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  if (!gulkan_queue_submit (queue, cmd_buffer))
    return FALSE;

  gulkan_queue_free_cmd_buffer (queue, cmd_buffer);

  return TRUE;
}

static gboolean
_init_target (GulkanFrameBuffer     *self,
              VkSampleCountFlagBits  sample_count,
              VkFormat               format,
              gboolean               is_depth_format,
              VkImageUsageFlags      usage,
              VkImage               *out_image,
              VkDeviceMemory        *out_memory)
{
  VkDevice vk_device = gulkan_device_get_handle (self->device);

  VkExternalMemoryImageCreateInfo external_info = {
    .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
    .pNext = NULL,
    .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
  };

  /* Depth/stencil target */
  VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = &external_info,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent = {
      .width = self->extent.width,
      .height = self->extent.height,
      .depth = 1,
    },
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .samples = sample_count,
    .usage = usage,
    .flags = 0,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
  };

  VkResult res;
  res = vkCreateImage (vk_device, &image_info, NULL, out_image);
  if (res != VK_SUCCESS)
    {
      g_print ("vkCreateImage failed for depth buffer: %d\n", res);
      return FALSE;
    }

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements (vk_device, *out_image, &memory_requirements);

  VkMemoryAllocateInfo memory_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size
  };
  if (!gulkan_device_memory_type_from_properties (
          self->device, memory_requirements.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          &memory_info.memoryTypeIndex))
    {
      g_print ("Failed to find memory type matching requirements.\n");
      return FALSE;
    }

  res = vkAllocateMemory (vk_device, &memory_info, NULL, out_memory);
  if (res != VK_SUCCESS)
    {
      g_print ("Failed to find memory for image.\n");
      return FALSE;
    }

  res = vkBindImageMemory (vk_device, *out_image, *out_memory, 0);
  if (res != VK_SUCCESS)
    {
      g_print ("Failed to bind memory for image.\n");
      return FALSE;
    }

  if (!_transfer_layout (*out_image, self->device, 1, is_depth_format,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL))
    {
      /* This is not fatal, but will result in validation errors */
      g_print ("Failed to transfer image layout to GENERAL\n");
    }

  return TRUE;
}

static gboolean
_init_frame_buffer (GulkanFrameBuffer *self, GulkanRenderPass *render_pass)
{
  VkDevice vk_device = gulkan_device_get_handle (self->device);

  VkRenderPass rp = gulkan_render_pass_get_handle (render_pass);

  VkFramebufferCreateInfo framebuffer_info = {
    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    .renderPass = rp,
    .attachmentCount = self->use_depth ? 2 : 1,
    .pAttachments = (VkImageView[]) {
      self->color_image_view,
      self->use_depth ? self->depth_stencil_image_view : NULL
    },
    .width = self->extent.width,
    .height = self->extent.height,
    .layers = 1
  };

  VkResult res = vkCreateFramebuffer (vk_device,
                                     &framebuffer_info, NULL,
                                     &self->framebuffer);
  if (res != VK_SUCCESS)
    {
      g_print ("vkCreateFramebuffer failed with error %d.\n", res);
      return FALSE;
    }
  return TRUE;
}

static gboolean
_initialize_from_image (GulkanFrameBuffer *self,
                        GulkanDevice      *device,
                        GulkanRenderPass  *render_pass,
                        VkImage            color_image,
                        VkExtent2D         extent,
                        VkFormat           color_format)
{
  self->device = device;
  self->extent = extent;
  self->use_depth = FALSE;

  if (!_create_image_view (self, color_image, color_format,
                           VK_IMAGE_ASPECT_COLOR_BIT,
                          &self->color_image_view))
    return FALSE;

  if (!_init_frame_buffer (self, render_pass))
    return FALSE;

  return TRUE;
}

static gboolean
_initialize_from_image_with_depth (GulkanFrameBuffer    *self,
                                   GulkanDevice         *device,
                                   GulkanRenderPass     *render_pass,
                                   VkImage               color_image,
                                   VkExtent2D            extent,
                                   VkSampleCountFlagBits sample_count,
                                   VkFormat              color_format)
{
  self->device = device;
  self->extent = extent;
  self->use_depth = TRUE;

  if (!_create_image_view (self, color_image, color_format,
                           VK_IMAGE_ASPECT_COLOR_BIT,
                          &self->color_image_view))
    return FALSE;


  VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
  if (!_init_target (self, sample_count,
                     depth_format, TRUE,
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                     &self->depth_stencil_image, &self->depth_stencil_memory))
    return FALSE;

  if (!_create_image_view (self, self->depth_stencil_image,
                           depth_format, VK_IMAGE_ASPECT_DEPTH_BIT,
                           &self->depth_stencil_image_view))
    return FALSE;


  if (!_init_frame_buffer (self, render_pass))
    return FALSE;

  return TRUE;
}

static gboolean
_initialize (GulkanFrameBuffer    *self,
             GulkanDevice         *device,
             GulkanRenderPass     *render_pass,
             VkExtent2D            extent,
             VkSampleCountFlagBits sample_count,
             VkFormat              color_format,
             gboolean              use_depth)
{
  VkImageUsageFlags usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  self->device = device;
  self->extent = extent;

  if (!_init_target (self, sample_count, color_format, FALSE,
                     usage, &self->color_image, &self->color_memory))
    return FALSE;

  if (use_depth)
    {
      if (!_initialize_from_image_with_depth (self, device, render_pass,
                                              self->color_image,
                                              extent,
                                              sample_count, color_format))
        return FALSE;
    }
  else
    {
      if (!_initialize_from_image (self, device, render_pass,
                                   self->color_image,
                                   extent, color_format))
        return FALSE;
    }

  return TRUE;
}

GulkanFrameBuffer *
gulkan_frame_buffer_new (GulkanDevice         *device,
                         GulkanRenderPass     *render_pass,
                         VkExtent2D            extent,
                         VkSampleCountFlagBits sample_count,
                         VkFormat              color_format,
                         gboolean              use_depth)
{
  GulkanFrameBuffer* self =
    (GulkanFrameBuffer*) g_object_new (GULKAN_TYPE_FRAME_BUFFER, 0);
  if (!_initialize (self, device, render_pass, extent,
                    sample_count, color_format, use_depth))
    {
      g_object_unref (self);
      return NULL;
    }
  return self;
}

GulkanFrameBuffer *
gulkan_frame_buffer_new_from_image_with_depth (GulkanDevice         *device,
                                               GulkanRenderPass     *render_pass,
                                               VkImage               color_image,
                                               VkExtent2D            extent,
                                               VkSampleCountFlagBits sample_count,
                                               VkFormat              color_format)
{
  GulkanFrameBuffer* self =
    (GulkanFrameBuffer*) g_object_new (GULKAN_TYPE_FRAME_BUFFER, 0);
  if (!_initialize_from_image_with_depth (self, device, render_pass,
                                          color_image,
                                          extent,
                                          sample_count, color_format))
    {
      g_object_unref (self);
      return NULL;
    }
  return self;
}

GulkanFrameBuffer *
gulkan_frame_buffer_new_from_image (GulkanDevice     *device,
                                    GulkanRenderPass *render_pass,
                                    VkImage           color_image,
                                    VkExtent2D        extent,
                                    VkFormat          color_format)
{
  GulkanFrameBuffer* self =
    (GulkanFrameBuffer*) g_object_new (GULKAN_TYPE_FRAME_BUFFER, 0);
  if (!_initialize_from_image (self, device, render_pass,
                               color_image, extent, color_format))
    {
      g_object_unref (self);
      return NULL;
    }
  return self;
}

VkImage
gulkan_frame_buffer_get_color_image (GulkanFrameBuffer *self)
{
  return self->color_image;
}

VkFramebuffer
gulkan_frame_buffer_get_handle (GulkanFrameBuffer *self)
{
  return self->framebuffer;
}
