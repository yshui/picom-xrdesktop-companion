/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_TEXTURE_H_
#define GULKAN_TEXTURE_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>
#include <vulkan/vulkan.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gulkan-buffer.h"
#include "gulkan-client.h"

G_BEGIN_DECLS

#define GULKAN_TYPE_TEXTURE gulkan_texture_get_type()
G_DECLARE_FINAL_TYPE (GulkanTexture, gulkan_texture,
                      GULKAN, TEXTURE, GObject)

GulkanTexture *
gulkan_texture_new_mip_levels (GulkanClient *client,
                               VkExtent2D    extent,
                               guint         mip_levels,
                               VkFormat      format);

GulkanTexture *
gulkan_texture_new (GulkanClient *client,
                    VkExtent2D    extent,
                    VkFormat      format);

GulkanTexture *
gulkan_texture_new_from_pixbuf (GulkanClient   *client,
                                GdkPixbuf      *pixbuf,
                                VkFormat        format,
                                VkImageLayout   layout,
                                gboolean        create_mipmaps);

GulkanTexture *
gulkan_texture_new_from_cairo_surface (GulkanClient    *client,
                                       cairo_surface_t *surface,
                                       VkFormat         format,
                                       VkImageLayout    layout);

GulkanTexture *
gulkan_texture_new_from_dmabuf (GulkanClient *client,
                                int           fd,
                                VkExtent2D    extent,
                                VkFormat      format);

GulkanTexture *
gulkan_texture_new_export_fd (GulkanClient *client,
                              VkExtent2D    extent,
                              VkFormat      format,
                              VkImageLayout layout,
                              gsize        *size,
                              int          *fd);

void
gulkan_texture_record_transfer (GulkanTexture       *self,
                                VkCommandBuffer      cmd_buffer,
                                VkImageLayout        src_layout,
                                VkImageLayout        dst_layout);

void
gulkan_texture_record_transfer_full (GulkanTexture       *self,
                                     VkCommandBuffer      cmd_buffer,
                                     VkAccessFlags        src_access_mask,
                                     VkAccessFlags        dst_access_mask,
                                     VkImageLayout        src_layout,
                                     VkImageLayout        dst_layout,
                                     VkPipelineStageFlags src_stage_mask,
                                     VkPipelineStageFlags dst_stage_mask);

gboolean
gulkan_texture_transfer_layout (GulkanTexture *self,
                                VkImageLayout  src_layout,
                                VkImageLayout  dst_layout);

gboolean
gulkan_texture_transfer_layout_full (GulkanTexture       *self,
                                     VkAccessFlags        src_access_mask,
                                     VkAccessFlags        dst_access_mask,
                                     VkImageLayout        src_layout,
                                     VkImageLayout        dst_layout,
                                     VkPipelineStageFlags src_stage_mask,
                                     VkPipelineStageFlags dst_stage_mask);

gboolean
gulkan_texture_upload_pixels (GulkanTexture  *self,
                              guchar         *pixels,
                              gsize           size,
                              VkImageLayout   layout);

gboolean
gulkan_texture_upload_cairo_surface (GulkanTexture   *self,
                                     cairo_surface_t *surface,
                                     VkImageLayout    layout);

gboolean
gulkan_texture_upload_pixbuf (GulkanTexture *self,
                              GdkPixbuf     *pixbuf,
                              VkImageLayout  layout);

VkImageView
gulkan_texture_get_image_view (GulkanTexture *self);

VkImage
gulkan_texture_get_image (GulkanTexture *self);

VkExtent2D
gulkan_texture_get_extent (GulkanTexture *self);

VkFormat
gulkan_texture_get_format (GulkanTexture *self);

guint
gulkan_texture_get_mip_levels (GulkanTexture *self);

G_END_DECLS

#endif /* GULKAN_TEXTURE_H_ */
