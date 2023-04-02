/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-pointer-tip.h"

#include "gxr-pointer-tip.h"
#include "xrd-scene-object.h"

typedef struct __attribute__((__packed__)) {
  float mvp[16];
  float mv[16];
  float m[16];
  bool receive_light;
} XrdScenePointerTipUniformBuffer;

typedef struct __attribute__((__packed__)) {
  float color[4];
  bool flip_y;
} XrdWindowUniformBuffer;

struct _XrdScenePointerTip
{
  GObject parent;

  GulkanClient *gulkan;
  VkBuffer lights;

  GulkanVertexBuffer *vertex_buffer;
  VkSampler sampler;
  float aspect_ratio;

  gboolean flip_y;
  graphene_vec3_t color;

  /* separate ubo used in fragment shader */
  GulkanUniformBuffer *shading_buffer;
  XrdWindowUniformBuffer shading_buffer_data;

  GulkanTexture *texture;
  uint32_t texture_width;
  uint32_t texture_height;

  float initial_width_meter;
  float initial_height_meter;
  float scale;

  GxrPointerTipData data;
};

static void
xrd_scene_pointer_tip_interface_init (GxrPointerTipInterface *iface);

G_DEFINE_TYPE_WITH_CODE (XrdScenePointerTip, xrd_scene_pointer_tip,
                         XRD_TYPE_SCENE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_POINTER_TIP,
                                                xrd_scene_pointer_tip_interface_init))

static void
xrd_scene_pointer_tip_finalize (GObject *gobject);

static void
xrd_scene_pointer_tip_class_init (XrdScenePointerTipClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_pointer_tip_finalize;
}

static void
xrd_scene_pointer_tip_init (XrdScenePointerTip *self)
{
  self->data.active = FALSE;
  self->data.animation = NULL;
  self->data.settings.width_meters = 1.0f;
  self->data.upload_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;



  self->vertex_buffer = gulkan_vertex_buffer_new ();
  self->sampler = VK_NULL_HANDLE;
  self->aspect_ratio = 1.0;
  self->shading_buffer_data.flip_y = FALSE;

  self->texture = NULL;
  self->texture_width = 0;
  self->texture_height = 0;
  self->initial_height_meter = 0;
  self->initial_width_meter = 0;
}

static void
_set_color (XrdScenePointerTip    *self,
            const graphene_vec3_t *color)
{
  graphene_vec3_init_from_vec3 (&self->color, color);

  graphene_vec4_t color_vec4;
  graphene_vec4_init_from_vec3 (&color_vec4, color, 1.0f);

  float color_arr[4];
  graphene_vec4_to_float (&color_vec4, color_arr);
  for (int i = 0; i < 4; i++)
    self->shading_buffer_data.color[i] = color_arr[i];

  gulkan_uniform_buffer_update (self->shading_buffer,
                                (gpointer) &self->shading_buffer_data);
}

static void
_append_plane (GulkanVertexBuffer *vbo, float aspect_ratio)
{
  graphene_matrix_t mat_scale;
  graphene_matrix_init_scale (&mat_scale, aspect_ratio, 1.0f, 1.0f);

  graphene_point_t from = { .x = -0.5, .y = -0.5 };
  graphene_point_t to = { .x = 0.5, .y = 0.5 };

  gulkan_geometry_append_plane (vbo, &from, &to, &mat_scale);
}

static gboolean
_initialize (XrdScenePointerTip* self, VkDescriptorSetLayout *layout)
{
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  GulkanDevice *device = gulkan_client_get_device (self->gulkan);

  _append_plane (self->vertex_buffer, self->aspect_ratio);
  if (!gulkan_vertex_buffer_alloc_array (self->vertex_buffer, device))
    return FALSE;

  VkDeviceSize ubo_size = sizeof (XrdScenePointerTipUniformBuffer);
  if (!xrd_scene_object_initialize (obj, self->gulkan, layout, ubo_size))
    return FALSE;

  self->shading_buffer =
    gulkan_uniform_buffer_new (device, sizeof (XrdWindowUniformBuffer));
  if (!self->shading_buffer)
    return FALSE;

  graphene_vec3_t white;
  graphene_vec3_init (&white, 1.0f, 1.0f, 1.0f);
  _set_color (self, &white);

  return TRUE;
}

static void
_scene_object_set_width_meters (XrdScenePointerTip *self,
                                float           width_meters)
{
  float height_meters = width_meters / self->aspect_ratio;

  self->initial_width_meter = width_meters;
  self->initial_height_meter = height_meters;
  self->scale = 1.0;

  xrd_scene_object_set_scale (XRD_SCENE_OBJECT (self), height_meters);
}

XrdScenePointerTip *
xrd_scene_pointer_tip_new (GulkanClient *gulkan,
                           VkDescriptorSetLayout *layout,
                           VkBuffer lights)
{
  XrdScenePointerTip* self =
    (XrdScenePointerTip*) g_object_new (XRD_TYPE_SCENE_POINTER_TIP, 0);

  self->gulkan = g_object_ref (gulkan);
  self->lights = lights;

  self->texture_width = 64;
  self->texture_height = 64;

  _initialize (self, layout);

  gxr_pointer_tip_init_settings (GXR_POINTER_TIP (self), &self->data);

  return self;
}

static void
xrd_scene_pointer_tip_finalize (GObject *gobject)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (gobject);

  /* cancels potentially running animation */
  gxr_pointer_tip_set_active (GXR_POINTER_TIP (self), FALSE);

  VkDevice device = gulkan_client_get_device_handle (self->gulkan);
  vkDestroySampler (device, self->sampler, NULL);

  g_object_unref (self->vertex_buffer);
  g_object_unref (self->shading_buffer);

  g_object_unref (self->texture);

  g_object_unref (self->gulkan);

  G_OBJECT_CLASS (xrd_scene_pointer_tip_parent_class)->finalize (gobject);
}

static void
_set_transformation (GxrPointerTip     *tip,
                     graphene_matrix_t *matrix)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  xrd_scene_object_set_transformation (XRD_SCENE_OBJECT (self), matrix);
}

static void
_get_transformation (GxrPointerTip     *tip,
                     graphene_matrix_t *matrix)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  graphene_matrix_t transformation;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self),
                                       &transformation);
  graphene_matrix_init_from_matrix (matrix, &transformation);
}


static void
_show (GxrPointerTip *tip)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  xrd_scene_object_show (obj);
}

static void
_hide (GxrPointerTip *tip)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  xrd_scene_object_hide (obj);
}

static gboolean
_is_visible (GxrPointerTip *tip)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  return xrd_scene_object_is_visible (obj);
}

static void
_set_width_meters (GxrPointerTip *tip,
                   float          meters)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  _scene_object_set_width_meters (self, meters);
}

static GulkanTexture*
_get_texture (GxrPointerTip *tip)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  return self->texture;
}

static void
_submit_texture (GxrPointerTip *tip)
{
  (void) tip;
}

static void
_update_descriptors (XrdScenePointerTip *self)
{
  VkDevice device = gulkan_client_get_device_handle (self->gulkan);

  for (uint32_t eye = 0; eye < 2; eye++)
  {
    VkBuffer transformation_buffer =
    xrd_scene_object_get_transformation_buffer (XRD_SCENE_OBJECT (self),
                                                eye);

    VkDescriptorSet descriptor_set =
    xrd_scene_object_get_descriptor_set (XRD_SCENE_OBJECT (self), eye);

    VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &(VkDescriptorBufferInfo) {
          .buffer = transformation_buffer,
          .offset = 0,
          .range = VK_WHOLE_SIZE
        },
        .pTexelBufferView = NULL
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &(VkDescriptorImageInfo) {
          .sampler = self->sampler,
          .imageView = gulkan_texture_get_image_view (self->texture),
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        .pBufferInfo = NULL,
        .pTexelBufferView = NULL
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &(VkDescriptorBufferInfo) {
          .buffer = gulkan_uniform_buffer_get_handle (self->shading_buffer),
          .offset = 0,
          .range = VK_WHOLE_SIZE
        },
        .pTexelBufferView = NULL
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 3,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &(VkDescriptorBufferInfo) {
          .buffer = self->lights,
          .offset = 0,
          .range = VK_WHOLE_SIZE
        },
        .pTexelBufferView = NULL
      }
    };

    vkUpdateDescriptorSets (device, 4, write_descriptor_sets, 0, NULL);
  }
}

static void
_set_and_submit_texture (GxrPointerTip *tip, GulkanTexture *texture)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  if (texture == self->texture)
    {
      g_debug ("Texture %p was already set on tip %p.\n",
                (void*) texture, (void*) self);
      return;
    }


  uint32_t previous_texture_width = self->texture_width;
  uint32_t previous_Texture_height = self->texture_height;

  VkExtent2D extent = gulkan_texture_get_extent (texture);
  self->texture_width = extent.width;
  self->texture_height = extent.height;

  VkDevice device = gulkan_client_get_device_handle (self->gulkan);

  float aspect_ratio = (float) extent.width / (float) extent.height;

  if (self->aspect_ratio != aspect_ratio)
    {
      self->aspect_ratio = aspect_ratio;
      gulkan_vertex_buffer_reset (self->vertex_buffer);
      _append_plane (self->vertex_buffer, self->aspect_ratio);
      gulkan_vertex_buffer_map_array (self->vertex_buffer);
    }

  /* self->texture == texture must be handled above */
  if (self->texture)
    g_object_unref (self->texture);

  self->texture = texture;
  guint mip_levels = gulkan_texture_get_mip_levels (texture);

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
    .maxLod = (float) mip_levels
  };

  if (self->sampler != VK_NULL_HANDLE)
    vkDestroySampler (device, self->sampler, NULL);

  vkCreateSampler (device, &sampler_info, NULL, &self->sampler);

  _update_descriptors (self);

  if (previous_texture_width != extent.width ||
    previous_Texture_height != extent.height)
  {
    /* initial-dims are respective the texture size and ppm.
     * Now that the texture size changed, initial dims need to be
     * updated, using the original ppm used to create this window. */
    float initial_width_meter = self->initial_width_meter;
    /* float initial_height_meter = self->initial_height_meter; */

    float previous_ppm = (float)previous_texture_width / initial_width_meter;
    float new_initial_width_meter = (float) extent.width / previous_ppm;

    /* updates "initial-width-meters"  and "initial height-meters"! */
    _scene_object_set_width_meters (self, new_initial_width_meter);
  }

}

static GxrPointerTipData*
_get_data (GxrPointerTip *tip)
{
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  return &self->data;
}

static GulkanClient*
_get_gulkan_client (GxrPointerTip *tip)
{
  (void) tip;
  XrdScenePointerTip *self = XRD_SCENE_POINTER_TIP (tip);
  return self->gulkan;
}

static void
_update_ubo (XrdScenePointerTip *self,
             GxrEye              eye,
             graphene_matrix_t  *vp)
{
  XrdScenePointerTipUniformBuffer ub;

  ub.receive_light = FALSE;

  graphene_matrix_t m_matrix;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&m_matrix, vp, &mvp_matrix);

  float mvp[16];
  graphene_matrix_to_float (&mvp_matrix, mvp);
  for (int i = 0; i < 16; i++)
    ub.mvp[i] = mvp[i];

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_pointer_tip_render (XrdScenePointerTip *self,
                              GxrEye              eye,
                              VkPipeline          pipeline,
                              VkPipelineLayout    pipeline_layout,
                              VkCommandBuffer     cmd_buffer,
                              graphene_matrix_t  *vp)
{
  if (!self->texture)
  {
    /* g_warning ("Trying to draw window with no texture.\n"); */
    return;
  }

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  _update_ubo (self, eye, vp);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}

static void
xrd_scene_pointer_tip_interface_init (GxrPointerTipInterface *iface)
{
  iface->set_transformation = _set_transformation;
  iface->get_transformation = _get_transformation;
  iface->show = _show;
  iface->hide = _hide;
  iface->is_visible = _is_visible;
  iface->set_width_meters = _set_width_meters;
  iface->submit_texture = _submit_texture;
  iface->set_and_submit_texture = _set_and_submit_texture;
  iface->get_texture = _get_texture;
  iface->get_data = _get_data;
  iface->get_gulkan_client = _get_gulkan_client;
}
