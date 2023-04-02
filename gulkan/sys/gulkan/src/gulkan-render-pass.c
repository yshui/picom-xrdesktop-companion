/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-render-pass.h"
#include "gulkan-frame-buffer.h"

struct _GulkanRenderPass
{
  GObject parent;

  GulkanDevice *device;

  VkRenderPass render_pass;

  gboolean use_depth;
};

G_DEFINE_TYPE (GulkanRenderPass, gulkan_render_pass, G_TYPE_OBJECT)

static void
_finalize (GObject *gobject)
{
  GulkanRenderPass *self = GULKAN_RENDER_PASS (gobject);

  VkDevice device = gulkan_device_get_handle (self->device);
  vkDestroyRenderPass (device, self->render_pass, NULL);

  g_object_unref (self->device);

  G_OBJECT_CLASS (gulkan_render_pass_parent_class)->finalize (gobject);
}

static void
gulkan_render_pass_class_init (GulkanRenderPassClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

static void
gulkan_render_pass_init (GulkanRenderPass *self)
{
  self->render_pass = VK_NULL_HANDLE;
}

static gboolean
_init (GulkanRenderPass     *self,
       GulkanDevice         *device,
       VkSampleCountFlagBits samples,
       VkFormat              color_format,
       VkImageLayout         final_color_layout,
       gboolean              use_depth)
{
  self->use_depth = use_depth;
  self->device = g_object_ref (device);

  VkDevice vk_device = gulkan_device_get_handle (self->device);

  VkAttachmentDescription *attachements = (VkAttachmentDescription[]) {
      {
        .format = color_format,
        .samples = samples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = final_color_layout,
        .flags = 0
      },
      {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = samples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .flags = 0
      },
  };

  VkAttachmentReference depth_attachement = {
    .attachment = 1,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkRenderPassCreateInfo renderpass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .flags = 0,
    .attachmentCount = self->use_depth ? 2 : 1,
    .pAttachments = attachements,
    .subpassCount = 1,
    .pSubpasses = &(VkSubpassDescription) {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &(VkAttachmentReference) {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      .pDepthStencilAttachment = self->use_depth ? &depth_attachement : NULL,
      .pResolveAttachments = NULL
     },
    .dependencyCount = 0,
    .pDependencies = NULL
  };

  VkResult res = vkCreateRenderPass (vk_device, &renderpass_info,
                                     NULL, &self->render_pass);
  vk_check_error ("vkCreateRenderPass", res, FALSE);

  return TRUE;
}

GulkanRenderPass *
gulkan_render_pass_new (GulkanDevice         *device,
                        VkSampleCountFlagBits samples,
                        VkFormat              color_format,
                        VkImageLayout         final_color_layout,
                        gboolean              use_depth)
{
  GulkanRenderPass *self =
    (GulkanRenderPass*) g_object_new (GULKAN_TYPE_RENDER_PASS, 0);

  if (!_init (self, device, samples, color_format,
              final_color_layout, use_depth))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

void
gulkan_render_pass_begin (GulkanRenderPass  *self,
                          VkExtent2D         extent,
                          VkClearColorValue  clear_color,
                          GulkanFrameBuffer *frame_buffer,
                          VkCommandBuffer    cmd_buffer)
{
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = self->render_pass,
    .framebuffer = gulkan_frame_buffer_get_handle (frame_buffer),
    .renderArea = {
      .offset = {
        .x = 0,
        .y = 0,
      },
      .extent = extent,
    },
    .clearValueCount = self->use_depth ? 2 : 1,
    .pClearValues = (VkClearValue[]) {
      {
        .color = clear_color
      },
      {
        .depthStencil = {
          .depth = 1.0f,
          .stencil = 0
        }
      },
    }
  };

  vkCmdBeginRenderPass (cmd_buffer, &render_pass_info,
                        VK_SUBPASS_CONTENTS_INLINE);
}

VkRenderPass
gulkan_render_pass_get_handle (GulkanRenderPass *self)
{
  return self->render_pass;
}
