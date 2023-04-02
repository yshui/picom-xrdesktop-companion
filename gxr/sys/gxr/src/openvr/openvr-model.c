/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-model.h"

#include <unistd.h>
#include "openvr-context.h"

void
openvr_model_print_list (void)
{
  OpenVRFunctions *f = openvr_get_functions ();

  uint32_t model_count = f->model->GetRenderModelCount ();
  char *name = g_malloc (sizeof (char) * k_unMaxPropertyStringSize);

  g_print ("You have %d render models:\n", model_count);
  for (uint32_t i = 0; i < model_count; i++)
    {
      uint32_t ret = f->model->GetRenderModelName (i, name,
                                                   k_unMaxPropertyStringSize);
      g_print ("\t%03d: %s\n", ret, name);
    }
  g_free (name);
}

GSList *
openvr_model_get_list (void)
{
  OpenVRFunctions *f = openvr_get_functions ();

  GSList *models = NULL;

  char *name = g_malloc (sizeof (char) * k_unMaxPropertyStringSize);
  for (uint32_t i = 0; i < f->model->GetRenderModelCount (); i++)
    {
      f->model->GetRenderModelName (i, name,k_unMaxPropertyStringSize);
      models = g_slist_append (models, g_strdup (name));
    }
  g_free (name);
  return models;
}

static gboolean
_load_openvr_mesh (RenderModel_t **model,
                   const char     *name)
{
  EVRRenderModelError error;
  OpenVRFunctions *f = openvr_get_functions ();

  do
    {
      char *name_cpy = g_strdup (name);
      error = f->model->LoadRenderModel_Async (name_cpy, model);
      g_free (name_cpy);
      /* Treat async loading synchronously.. */
      g_usleep (1000);
    }
  while (error == EVRRenderModelError_VRRenderModelError_Loading);

  if (error != EVRRenderModelError_VRRenderModelError_None)
    {
      g_printerr ("Unable to load model %s - %s\n", name,
                  f->model->GetRenderModelErrorNameFromEnum (error));
      return FALSE;
    }

  return TRUE;
}

static gboolean
_load_openvr_texture (TextureID_t                id,
                      RenderModel_TextureMap_t **texture)
{
  OpenVRFunctions *f = openvr_get_functions ();

  EVRRenderModelError error;
  do
    {
      error = f->model->LoadTexture_Async (id, texture);
      /* Treat async loading synchronously.. */
      g_usleep (1000);
    }
  while (error == EVRRenderModelError_VRRenderModelError_Loading);

  if (error != EVRRenderModelError_VRRenderModelError_None)
    {
      g_printerr ("Unable to load OpenVR texture id: %d\n", id);
      return FALSE;
    }

  return TRUE;
}

static gboolean
_load_mesh (GulkanVertexBuffer *vbo,
            GulkanDevice       *device,
            RenderModel_t      *vr_model)
{
  if (!gulkan_vertex_buffer_alloc_data (
      vbo, device, vr_model->rVertexData,
      sizeof (RenderModel_Vertex_t) * vr_model->unVertexCount))
    return FALSE;

  if (!gulkan_vertex_buffer_alloc_index_data (
      vbo, device, vr_model->rIndexData,
      sizeof (uint16_t), vr_model->unTriangleCount * 3))
    return FALSE;

  return TRUE;
}

static gboolean
_load_texture (GulkanTexture            **texture,
               VkSampler                *sampler,
               GulkanClient             *gc,
               RenderModel_TextureMap_t *texture_map)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (
      texture_map->rubTextureMapData, GDK_COLORSPACE_RGB, TRUE, 8,
      texture_map->unWidth, texture_map->unHeight,
      4 * texture_map->unWidth, NULL, NULL);

  *texture =
    gulkan_texture_new_from_pixbuf (gc, pixbuf,
                                    VK_FORMAT_R8G8B8A8_SRGB,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    TRUE);
  g_object_unref (pixbuf);

  guint mip_levels = gulkan_texture_get_mip_levels (*texture);

  VkSamplerCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = 16.0f,
    .minLod = 0.0f,
    .maxLod = (float) mip_levels,
  };

  GulkanDevice *device = gulkan_client_get_device (gc);
  vkCreateSampler (gulkan_device_get_handle (device), &sampler_info, NULL,
                   sampler);

  return TRUE;
}

gboolean
openvr_model_load (GulkanClient       *gc,
                   GulkanVertexBuffer *vbo,
                   GulkanTexture      **texture,
                   VkSampler          *sampler,
                   const char         *model_name)
{
  RenderModel_t *vr_model;
  if (!_load_openvr_mesh (&vr_model, model_name))
    return FALSE;

  OpenVRFunctions *f = openvr_get_functions ();

  RenderModel_TextureMap_t *vr_diffuse_texture;
  if (!_load_openvr_texture (vr_model->diffuseTextureId, &vr_diffuse_texture))
    {
      f->model->FreeRenderModel (vr_model);
      return FALSE;
    }

  GulkanDevice *device = gulkan_client_get_device (gc);
  if (!_load_mesh (vbo, device, vr_model))
    return FALSE;

  if (!_load_texture (texture, sampler, gc, vr_diffuse_texture))
    return FALSE;

  f->model->FreeRenderModel (vr_model);
  f->model->FreeTexture (vr_diffuse_texture);

  return TRUE;
}

uint32_t
openvr_model_get_vertex_stride ()
{
  return sizeof (RenderModel_Vertex_t);
}

uint32_t
openvr_model_get_normal_offset ()
{
  return offsetof (RenderModel_Vertex_t, vNormal);
}

uint32_t
openvr_model_get_uv_offset ()
{
  return offsetof (RenderModel_Vertex_t, rfTextureCoord);
}
