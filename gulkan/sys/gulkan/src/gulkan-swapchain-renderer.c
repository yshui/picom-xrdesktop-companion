/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-swapchain-renderer.h"
#include "gulkan-frame-buffer.h"
#include "gulkan-swapchain.h"

typedef struct RenderBuffer
{
  GulkanFrameBuffer *fb;
  VkFence fence;
  VkCommandBuffer cmd_buffer;
} RenderBuffer;

typedef struct _GulkanSwapchainRendererPrivate
{
  GulkanRenderer parent;

  RenderBuffer *buffers;

  GulkanRenderPass *pass;
  GulkanSwapchain *swapchain;

  VkClearColorValue clear_color;

  VkSemaphore acquire_to_submit_semaphore;
  VkSemaphore submit_to_present_semaphore;

  VkFormat format;

} GulkanSwapchainRendererPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GulkanSwapchainRenderer, gulkan_swapchain_renderer,
                            GULKAN_TYPE_RENDERER)

static void
gulkan_swapchain_renderer_init (GulkanSwapchainRenderer *self)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);
  priv->format = VK_FORMAT_B8G8R8A8_SRGB;
}

static void
_finalize (GObject *gobject)
{
  GulkanSwapchainRenderer *self = GULKAN_SWAPCHAIN_RENDERER (gobject);
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  GulkanClient *client = gulkan_renderer_get_client (GULKAN_RENDERER (self));
  if (client)
    {
      VkDevice device = gulkan_client_get_device_handle (client);

      if (priv->swapchain)
        {
          for (uint32_t i = 0;
               i < gulkan_swapchain_get_size (priv->swapchain); i++)
          {
            g_object_unref (priv->buffers[i].fb);
            vkDestroyFence (device, priv->buffers[i].fence, NULL);
          }
          g_object_unref (priv->swapchain);
        }

      vkDestroySemaphore (device, priv->acquire_to_submit_semaphore, NULL);
      vkDestroySemaphore (device, priv->submit_to_present_semaphore, NULL);

      g_clear_object (&priv->pass);
      g_free (priv->buffers);
    }

  G_OBJECT_CLASS (gulkan_swapchain_renderer_parent_class)->finalize (gobject);
}

static gboolean
_init_sync (GulkanSwapchainRenderer *self)
{
  GulkanClient *client = gulkan_renderer_get_client (GULKAN_RENDERER (self));
  VkDevice device = gulkan_client_get_device_handle (client);

  VkSemaphoreCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  VkResult res;
  res = vkCreateSemaphore (device, &info, NULL,
                          &priv->acquire_to_submit_semaphore);
  vk_check_error ("vkCreateSemaphore", res, TRUE);
  res = vkCreateSemaphore (device, &info, NULL,
                          &priv->submit_to_present_semaphore);
  vk_check_error ("vkCreateSemaphore", res, TRUE);

  return TRUE;
}

static gboolean
_init_render_buffer (GulkanSwapchainRenderer *self, RenderBuffer *b, VkImage image)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  GulkanClient *client = gulkan_renderer_get_client (GULKAN_RENDERER (self));
  GulkanDevice *gulkan_device = gulkan_client_get_device (client);
  VkDevice device = gulkan_device_get_handle (gulkan_device);
  VkExtent2D extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self));
  VkFormat format = gulkan_swapchain_get_format (priv->swapchain);

  b->fb = gulkan_frame_buffer_new_from_image (gulkan_device, priv->pass, image,
                                              extent, format);

  if (!b->fb)
    return FALSE;

  VkResult res = vkCreateFence (
    device,
    &(VkFenceCreateInfo){ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                          .flags = VK_FENCE_CREATE_SIGNALED_BIT },
    NULL, &b->fence);
  vk_check_error ("vkCreateFence", res, FALSE);

  GulkanQueue *gulkan_queue = gulkan_device_get_graphics_queue (gulkan_device);
  VkCommandPool pool = gulkan_queue_get_command_pool (gulkan_queue);
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  res = vkAllocateCommandBuffers (device, &alloc_info, &b->cmd_buffer);
  vk_check_error ("vkAllocateCommandBuffers", res, FALSE);

  return TRUE;
}

static gboolean
_init_render_buffers (GulkanSwapchainRenderer *self)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  uint32_t size = gulkan_swapchain_get_size (priv->swapchain);
  VkImage *images = g_malloc (sizeof (VkImage) * size);
  gulkan_swapchain_get_images (priv->swapchain, images);

  priv->buffers = g_malloc (sizeof (RenderBuffer) * size);

  for (uint32_t i = 0; i < size; i++)
    if (!_init_render_buffer (self, &priv->buffers[i], images[i]))
      return FALSE;

  g_free (images);

  return TRUE;
}

VkRenderPass
gulkan_swapchain_renderer_get_render_pass_handle (GulkanSwapchainRenderer *self)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  return gulkan_render_pass_get_handle (priv->pass);
}

GulkanRenderPass *
gulkan_swapchain_renderer_get_render_pass (GulkanSwapchainRenderer *self)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  return priv->pass;
}

uint32_t
gulkan_swapchain_renderer_get_swapchain_size (GulkanSwapchainRenderer *self)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  return gulkan_swapchain_get_size (priv->swapchain);
}

VkCommandBuffer
gulkan_swapchain_renderer_get_cmd_buffer (GulkanSwapchainRenderer *self,
                                          uint32_t index)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  return priv->buffers[index].cmd_buffer;
}

GulkanFrameBuffer *
gulkan_swapchain_renderer_get_frame_buffer (GulkanSwapchainRenderer *self,
                                            uint32_t index)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  return priv->buffers[index].fb;
}

static gboolean
_draw (GulkanRenderer *renderer)
{
  GulkanSwapchainRenderer *self = GULKAN_SWAPCHAIN_RENDERER (renderer);
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  uint32_t index;
  if (!gulkan_swapchain_acquire (priv->swapchain,
                                 priv->acquire_to_submit_semaphore, &index))
    return TRUE;

  g_assert (index <= gulkan_swapchain_get_size (priv->swapchain));

  RenderBuffer *b = &priv->buffers[index];

  GulkanClient *client = gulkan_renderer_get_client (GULKAN_RENDERER (self));
  GulkanDevice *gulkan_device = gulkan_client_get_device (client);
  VkDevice device = gulkan_device_get_handle (gulkan_device);

  VkResult res;
  res = vkWaitForFences (device, 1, &b->fence, VK_TRUE, UINT64_MAX);
  vk_check_error ("vkWaitForFences", res, FALSE);

  res = vkResetFences (device, 1, &b->fence);
  vk_check_error ("vkResetFences", res, FALSE);

  GulkanQueue *gulkan_queue = gulkan_device_get_graphics_queue (gulkan_device);
  VkQueue queue = gulkan_queue_get_handle (gulkan_queue);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &priv->acquire_to_submit_semaphore,
    .pWaitDstStageMask =
      (VkPipelineStageFlags[]){
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      },
    .commandBufferCount = 1,
    .pCommandBuffers = &b->cmd_buffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &priv->submit_to_present_semaphore,
  };

  res = vkQueueSubmit (queue, 1, &submit_info, b->fence);
  vk_check_error ("vkQueueSubmit", res, FALSE);

  gulkan_swapchain_present (priv->swapchain,
                           &priv->submit_to_present_semaphore, index);

  return TRUE;
}

gboolean
gulkan_swapchain_renderer_init_draw_cmd_buffers (GulkanSwapchainRenderer *self)
{
  GulkanSwapchainRendererClass *klass =
    GULKAN_SWAPCHAIN_RENDERER_GET_CLASS (self);
  if (klass->init_draw_cmd == NULL)
    return FALSE;

  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  VkResult res;

  VkCommandBufferBeginInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = 0
  };

  GulkanSwapchainRenderer *scr = GULKAN_SWAPCHAIN_RENDERER (self);
  uint32_t size = gulkan_swapchain_renderer_get_swapchain_size (scr);

  VkExtent2D extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self));

  const VkViewport viewport = {
    .x = 0,
    .y = 0,
    .width = extent.width,
    .height = extent.height,
    .minDepth = 0,
    .maxDepth = 1,
  };
  VkRect2D render_area = { { 0, 0 }, extent };

  for (uint32_t i = 0; i < size; i++)
    {
      VkCommandBuffer cmd_buffer =
        gulkan_swapchain_renderer_get_cmd_buffer (scr, i);
      res = vkBeginCommandBuffer (cmd_buffer, &info);
      vk_check_error ("vkBeginCommandBuffer", res, FALSE);

      gulkan_swapchain_renderer_begin_render_pass (scr, priv->clear_color, i);

      vkCmdSetViewport (cmd_buffer, 0, 1, &viewport);
      vkCmdSetScissor (cmd_buffer, 0, 1, &render_area);

      klass->init_draw_cmd (self, cmd_buffer);

      vkCmdEndRenderPass (cmd_buffer);

      res = vkEndCommandBuffer (cmd_buffer);
      vk_check_error ("vkEndCommandBuffer", res, FALSE);
    }

  return TRUE;
}

gboolean
gulkan_swapchain_renderer_initialize (GulkanSwapchainRenderer *self,
                                      VkSurfaceKHR             surface,
                                      VkClearColorValue        clear_color,
                                      gconstpointer            pipeline_data)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  priv->clear_color = clear_color;

  GulkanClient *client = gulkan_renderer_get_client (GULKAN_RENDERER (self));
  GulkanDevice *gulkan_device = gulkan_client_get_device (client);

  priv->swapchain = gulkan_swapchain_new (
    client, surface, VK_PRESENT_MODE_FIFO_KHR, priv->format,
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
  if (!priv->swapchain)
    {
      g_printerr ("Could not init swapchain.\n");
      return FALSE;
    }

  gulkan_renderer_set_extent (GULKAN_RENDERER (self),
                              gulkan_swapchain_get_extent (priv->swapchain));

  priv->pass =
    gulkan_render_pass_new (gulkan_device, VK_SAMPLE_COUNT_1_BIT,
                            gulkan_swapchain_get_format (priv->swapchain),
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, FALSE);
  if (!priv->pass)
    {
      g_printerr ("Could not init render pass.\n");
      return FALSE;
    }

  if (!_init_render_buffers (self))
    return FALSE;

  if (!_init_sync (self))
    return FALSE;

  GulkanSwapchainRendererClass *klass =
    GULKAN_SWAPCHAIN_RENDERER_GET_CLASS (self);
  if (klass->init_pipeline == NULL)
    return FALSE;

  if (!klass->init_pipeline (self, pipeline_data))
    return FALSE;

  if (!gulkan_swapchain_renderer_init_draw_cmd_buffers (self))
    return FALSE;

  return TRUE;
}

void
gulkan_swapchain_renderer_begin_render_pass (GulkanSwapchainRenderer *self,
                                             VkClearColorValue        clear_color,
                                             uint32_t                 index)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  VkExtent2D extent = gulkan_renderer_get_extent (GULKAN_RENDERER (self));

  RenderBuffer *b = &priv->buffers[index];

  gulkan_render_pass_begin (priv->pass, extent, clear_color,
                            b->fb, b->cmd_buffer);
}

static void
gulkan_swapchain_renderer_class_init (GulkanSwapchainRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;

  GulkanRendererClass *parent_class = GULKAN_RENDERER_CLASS (klass);
  parent_class->draw = _draw;
}

gboolean
gulkan_swapchain_renderer_resize (GulkanSwapchainRenderer *self,
                                  VkSurfaceKHR             surface)
{
  GulkanSwapchainRendererPrivate *priv =
    gulkan_swapchain_renderer_get_instance_private (self);

  GulkanClient *client = gulkan_renderer_get_client (GULKAN_RENDERER (self));
  GulkanDevice *gulkan_device = gulkan_client_get_device (client);


  for (uint32_t i = 0; i < gulkan_swapchain_get_size (priv->swapchain); i++)
    g_object_unref (priv->buffers[i].fb);
  g_object_unref (priv->swapchain);

  priv->swapchain = gulkan_swapchain_new (
    client, surface, VK_PRESENT_MODE_FIFO_KHR, priv->format,
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

  if (!priv->swapchain)
    return FALSE;

  VkExtent2D extent = gulkan_swapchain_get_extent (priv->swapchain);

  gulkan_renderer_set_extent (GULKAN_RENDERER (self), extent);

  size_t size = gulkan_swapchain_get_size (priv->swapchain);
  VkImage *images = g_malloc (sizeof(VkImage) * size);
  gulkan_swapchain_get_images (priv->swapchain, images);

  VkFormat format = gulkan_swapchain_get_format (priv->swapchain);

  GulkanQueue *gulkan_queue = gulkan_device_get_graphics_queue (gulkan_device);
  VkCommandPool pool = gulkan_queue_get_command_pool (gulkan_queue);

  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  VkDevice device = gulkan_device_get_handle (gulkan_device);

  for (uint32_t i = 0; i < size; i++)
    {
      RenderBuffer *b = &priv->buffers[i];
      b->fb = gulkan_frame_buffer_new_from_image (gulkan_device, priv->pass,
                                                  images[i], extent, format);
      if (!b->fb)
        {
          g_printerr ("Error: Creating framebuffer failed.\n");
          return FALSE;
        }

      /* Redo command buffers */
      VkResult res = vkAllocateCommandBuffers (device, &alloc_info,
                                               &b->cmd_buffer);
      vk_check_error ("vkAllocateCommandBuffers", res, FALSE);
    }

  g_free (images);

  if (!gulkan_swapchain_renderer_init_draw_cmd_buffers (self))
    return FALSE;

  return TRUE;
}
