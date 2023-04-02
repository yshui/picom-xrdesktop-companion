/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gulkan.h>

#include "graphene-ext.h"
#include "xrd-scene-window-private.h"
#include "xrd-scene-renderer.h"
#include "xrd-render-lock.h"

enum
{
  PROP_TITLE = 1,
  PROP_SCALE,
  PROP_NATIVE,
  PROP_TEXTURE_WIDTH,
  PROP_TEXTURE_HEIGHT,
  PROP_WIDTH_METERS,
  PROP_HEIGHT_METERS,
  N_PROPERTIES
};

static void
xrd_scene_window_window_interface_init (XrdWindowInterface *iface);

typedef struct __attribute__((__packed__)) {
  float color[4];
  bool flip_y;
} XrdWindowShadingUniformBuffer;

typedef struct __attribute__((__packed__)) {
  float mvp[16];
  float mv[16];
  float m[16];
  bool receive_light;
} XrdSceneWindowUniformBuffer;

typedef struct _XrdSceneWindowPrivate
{
  XrdSceneObject parent;

  GulkanClient *gulkan;
  VkBuffer lights;

  GulkanVertexBuffer *vertex_buffer;
  VkSampler sampler;
  float aspect_ratio;

  gboolean flip_y;
  graphene_vec3_t color;

  GulkanUniformBuffer *shading_buffer;
  XrdWindowShadingUniformBuffer shading_buffer_data;

  XrdWindowData *window_data;
} XrdSceneWindowPrivate;

G_DEFINE_TYPE_WITH_CODE (XrdSceneWindow, xrd_scene_window, XRD_TYPE_SCENE_OBJECT,
                         G_ADD_PRIVATE (XrdSceneWindow)
                         G_IMPLEMENT_INTERFACE (XRD_TYPE_WINDOW,
                                                xrd_scene_window_window_interface_init))

static void
xrd_scene_window_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (object);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);

  switch (property_id)
    {
    case PROP_TITLE:
      if (priv->window_data->title)
        g_string_free (priv->window_data->title, TRUE);
      priv->window_data->title = g_string_new (g_value_get_string (value));
      break;
    case PROP_SCALE:
      priv->window_data->scale = g_value_get_float (value);
      break;
    case PROP_NATIVE:
      priv->window_data->native = g_value_get_pointer (value);
      break;
    case PROP_TEXTURE_WIDTH:
      priv->window_data->texture_width = g_value_get_uint (value);
      break;
    case PROP_TEXTURE_HEIGHT:
      priv->window_data->texture_height = g_value_get_uint (value);
      break;
    case PROP_WIDTH_METERS:
      priv->window_data->initial_size_meters.x = g_value_get_float (value);
      break;
    case PROP_HEIGHT_METERS:
      priv->window_data->initial_size_meters.y = g_value_get_float (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
xrd_scene_window_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (object);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->window_data->title->str);
      break;
    case PROP_SCALE:
      g_value_set_float (value, priv->window_data->scale);
      break;
    case PROP_NATIVE:
      g_value_set_pointer (value, priv->window_data->native);
      break;
    case PROP_TEXTURE_WIDTH:
      g_value_set_uint (value, priv->window_data->texture_width);
      break;
    case PROP_TEXTURE_HEIGHT:
      g_value_set_uint (value, priv->window_data->texture_height);
      break;
    case PROP_WIDTH_METERS:
      g_value_set_float (value, priv->window_data->initial_size_meters.x);
      break;
    case PROP_HEIGHT_METERS:
      g_value_set_float (value, priv->window_data->initial_size_meters.y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
xrd_scene_window_finalize (GObject *gobject);

static void
xrd_scene_window_class_init (XrdSceneWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_window_finalize;

  object_class->set_property = xrd_scene_window_set_property;
  object_class->get_property = xrd_scene_window_get_property;

  g_object_class_override_property (object_class, PROP_TITLE, "title");
  g_object_class_override_property (object_class, PROP_SCALE, "scale");
  g_object_class_override_property (object_class, PROP_NATIVE, "native");
  g_object_class_override_property (object_class, PROP_TEXTURE_WIDTH, "texture-width");
  g_object_class_override_property (object_class, PROP_TEXTURE_HEIGHT, "texture-height");
  g_object_class_override_property (object_class, PROP_WIDTH_METERS, "initial-width-meters");
  g_object_class_override_property (object_class, PROP_HEIGHT_METERS, "initial-height-meters");
}

static void
xrd_scene_window_init (XrdSceneWindow *self)
{
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  priv->window_data = g_malloc (sizeof (XrdWindowData));

  priv->vertex_buffer = gulkan_vertex_buffer_new ();
  priv->sampler = VK_NULL_HANDLE;
  priv->aspect_ratio = 1.0;
  priv->window_data->texture = NULL;
  priv->shading_buffer_data.flip_y = FALSE;

  priv->window_data->title = NULL;
  priv->window_data->child_window = NULL;
  priv->window_data->parent_window = NULL;
  priv->window_data->native = NULL;
  priv->window_data->texture_width = 0;
  priv->window_data->texture_height = 0;
  priv->window_data->texture = NULL;
  priv->window_data->xrd_window = XRD_WINDOW (self);
  priv->window_data->pinned = FALSE;

  priv->window_data->owned_by_window = TRUE;

  priv->gulkan = NULL;

  graphene_matrix_init_identity (&priv->window_data->reset_transform);
}

static XrdSceneWindow *
_new (const gchar           *title,
      GulkanClient          *gulkan,
      VkDescriptorSetLayout *layout,
      VkBuffer               lights)
{
  XrdSceneWindow* self = (XrdSceneWindow*) g_object_new (XRD_TYPE_SCENE_WINDOW,
                                                         "title", title, NULL);
  if (!xrd_scene_window_initialize (self, gulkan, layout, lights))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

XrdSceneWindow *
xrd_scene_window_new_from_meters (const gchar           *title,
                                  float                  width,
                                  float                  height,
                                  float                  ppm,
                                  GulkanClient          *gulkan,
                                  VkDescriptorSetLayout *layout,
                                  VkBuffer               lights)
{
  XrdSceneWindow *window = _new (title, gulkan, layout, lights);
  g_object_set (window,
                "texture-width", (uint32_t) (width * ppm),
                "texture-height", (uint32_t) (height * ppm),
                "initial-width-meters", (double) width,
                "initial-height-meters", (double) height,
                NULL);
  return window;
}

static gboolean
_set_transformation (XrdWindow         *window,
                     graphene_matrix_t *mat);

XrdSceneWindow *
xrd_scene_window_new_from_data (XrdWindowData         *data,
                                GulkanClient          *gulkan,
                                VkDescriptorSetLayout *layout,
                                VkBuffer               lights)
{
  XrdSceneWindow *window = _new (data->title->str, gulkan, layout, lights);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (window);

  // TODO: avoid unnecessary allocation
  g_free (priv->window_data);

  priv->window_data = data;

  _set_transformation (XRD_WINDOW (window), &data->transform);

  return window;
}

XrdSceneWindow *
xrd_scene_window_new_from_pixels (const gchar           *title,
                                  uint32_t               width,
                                  uint32_t               height,
                                  float                  ppm,
                                  GulkanClient          *gulkan,
                                  VkDescriptorSetLayout *layout,
                                  VkBuffer               lights)
{
  XrdSceneWindow *window = _new (title, gulkan, layout, lights);
  g_object_set (window,
                "texture-width", width,
                "texture-height", height,
                "initial-width-meters", (double) width / (double) ppm,
                "initial-height-meters", (double) height / (double) ppm,
                NULL);
  return window;
}

XrdSceneWindow *
xrd_scene_window_new_from_native (const gchar           *title,
                                  gpointer               native,
                                  uint32_t               width_pixels,
                                  uint32_t               height_pixels,
                                  float                  ppm,
                                  GulkanClient          *gulkan,
                                  VkDescriptorSetLayout *layout,
                                  VkBuffer               lights)
{
  XrdSceneWindow *window = xrd_scene_window_new_from_pixels (title,
                                                             width_pixels,
                                                             height_pixels,
                                                             ppm,
                                                             gulkan,
                                                             layout,
                                                             lights);
  g_object_set (window, "native", native, NULL);
  return window;
}


static void
xrd_scene_window_finalize (GObject *gobject)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (gobject);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);

  xrd_render_lock ();
  if (priv->window_data->texture)
    g_clear_object (&priv->window_data->texture);
  xrd_render_unlock ();

  vkDestroySampler (gulkan_client_get_device_handle (priv->gulkan),
                    priv->sampler, NULL);
  g_clear_object (&priv->vertex_buffer);
  g_clear_object (&priv->shading_buffer);

  g_clear_object (&priv->gulkan);

  if (priv->window_data->owned_by_window)
    {
      if (priv->window_data->title)
        g_string_free (priv->window_data->title, TRUE);
      g_free (priv->window_data);
    }

  G_OBJECT_CLASS (xrd_scene_window_parent_class)->finalize (gobject);
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

gboolean
xrd_scene_window_initialize (XrdSceneWindow        *self,
                             GulkanClient          *gulkan,
                             VkDescriptorSetLayout *layout,
                             VkBuffer               lights)
{
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  priv->gulkan = g_object_ref (gulkan);
  priv->lights = lights;

  GulkanDevice *device = gulkan_client_get_device (priv->gulkan);

  _append_plane (priv->vertex_buffer, priv->aspect_ratio);
  if (!gulkan_vertex_buffer_alloc_array (priv->vertex_buffer, device))
    return FALSE;

  VkDeviceSize ubo_size = sizeof (XrdSceneWindowUniformBuffer);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_initialize (obj, priv->gulkan, layout, ubo_size))
    return FALSE;

  VkDeviceSize shading_ubo_size = sizeof (XrdWindowShadingUniformBuffer);

  priv->shading_buffer = gulkan_uniform_buffer_new (device, shading_ubo_size);
  if (!priv->shading_buffer)
    return FALSE;

  graphene_vec3_t white;
  graphene_vec3_init (&white, 1.0f, 1.0f, 1.0f);
  xrd_scene_window_set_color (self, &white);

  return TRUE;
}

static void
_update_ubo (XrdSceneWindow    *self,
             GxrEye             eye,
             graphene_matrix_t *view_matrix,
             graphene_matrix_t *projection_matrix,
             gboolean           shaded)
{
  XrdSceneWindowUniformBuffer ub;

  graphene_matrix_t m_matrix;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mv_matrix;
  graphene_matrix_multiply (&m_matrix, view_matrix, &mv_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&mv_matrix, projection_matrix, &mvp_matrix);

  float mvp[16];
  graphene_matrix_to_float (&mvp_matrix, mvp);

  float mv[16];
  graphene_matrix_to_float (&mv_matrix, mv);

  float m[16];
  graphene_matrix_to_float (&m_matrix, m);

  for (int i = 0; i < 16; i++)
    {
      ub.mvp[i] = mvp[i];
      ub.mv[i] = mv[i];
      ub.m[i] = m[i];
      ub.receive_light = shaded;
    }

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_window_render (XrdSceneWindow    *self,
                         GxrEye             eye,
                         VkPipeline         pipeline,
                         VkPipelineLayout   pipeline_layout,
                         VkCommandBuffer    cmd_buffer,
                         graphene_matrix_t *view,
                         graphene_matrix_t *projection,
                         gboolean           shaded)
{
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  if (!priv->window_data->texture)
    {
      /* g_warning ("Trying to draw window with no texture.\n"); */
      return;
    }

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    return;

  _update_ubo (self, eye, view, projection, shaded);

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (priv->vertex_buffer, cmd_buffer);
}

void
xrd_scene_window_set_color (XrdSceneWindow        *self,
                            const graphene_vec3_t *color)
{
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  graphene_vec3_init_from_vec3 (&priv->color, color);

  graphene_vec4_t color_vec4;
  graphene_vec4_init_from_vec3 (&color_vec4, color, 1.0f);

  float color_array[4];
  graphene_vec4_to_float (&color_vec4, color_array);

  for (uint32_t i = 0; i < 4; i++)
    priv->shading_buffer_data.color[i] = color_array[i];

  gulkan_uniform_buffer_update (priv->shading_buffer,
                                (gpointer) &priv->shading_buffer_data);
}

void
xrd_scene_window_update_descriptors (XrdSceneWindow *self)
{
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  VkDevice device = gulkan_client_get_device_handle (priv->gulkan);

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
            .sampler = priv->sampler,
            .imageView = gulkan_texture_get_image_view (priv->window_data->texture),
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
            .buffer = gulkan_uniform_buffer_get_handle (priv->shading_buffer),
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
            .buffer = priv->lights,
            .offset = 0,
            .range = VK_WHOLE_SIZE
          },
          .pTexelBufferView = NULL
        }
      };

      vkUpdateDescriptorSets (device, 4, write_descriptor_sets, 0, NULL);
    }
}

/* XrdWindow Interface functions */

static gboolean
_set_transformation (XrdWindow         *window,
                     graphene_matrix_t *mat)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);
  xrd_scene_object_set_transformation (XRD_SCENE_OBJECT (self), mat);

  float height_meters =
    xrd_window_get_current_height_meters (XRD_WINDOW (self));

  xrd_scene_object_set_scale (XRD_SCENE_OBJECT (self), height_meters);

  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  if (priv->window_data->child_window)
    xrd_window_update_child (window);

  XrdWindowData *data = xrd_window_get_data (window);
  graphene_matrix_t transform_unscaled;
  xrd_window_get_transformation_no_scale (window, &transform_unscaled);
  graphene_matrix_init_from_matrix (&data->transform, &transform_unscaled);

  return TRUE;
}

static gboolean
_get_transformation (XrdWindow         *window,
                     graphene_matrix_t *mat)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), mat);
  return TRUE;
}

static gboolean
_get_transformation_no_scale (XrdWindow         *window,
                              graphene_matrix_t *mat)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);
  *mat = xrd_scene_object_get_transformation_no_scale (XRD_SCENE_OBJECT (self));
  return TRUE;
}

static void
_submit_texture (XrdWindow *window)
{
  (void) window;
}

static void
_set_and_submit_texture (XrdWindow *window, GulkanTexture *texture)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);

  xrd_render_lock ();

  if (texture == priv->window_data->texture)
    {
      gchar *title;
      g_object_get (window, "title", &title, NULL);
      g_debug ("Texture %p was already set on window %p (%s).\n",
               (void*) texture, (void*) window, title);

      xrd_render_unlock ();
      return;
    }


  uint32_t previous_texture_width, previous_Texture_height;
  g_object_get (self,
                "texture-width", &previous_texture_width,
                "texture-height", &previous_Texture_height,
                NULL);

  VkExtent2D extent = gulkan_texture_get_extent (texture);
  g_object_set (window,
                "texture-width", extent.width,
                "texture-height", extent.height,
                NULL);

  VkDevice device = gulkan_client_get_device_handle (priv->gulkan);

  float aspect_ratio = (float) extent.width / (float) extent.height;

  if (priv->aspect_ratio != aspect_ratio)
    {
      priv->aspect_ratio = aspect_ratio;
      gulkan_vertex_buffer_reset (priv->vertex_buffer);
      _append_plane (priv->vertex_buffer, priv->aspect_ratio);
      gulkan_vertex_buffer_map_array (priv->vertex_buffer);
    }

  /* priv->window_data->texture == texture must be handled above */
  if (priv->window_data->texture)
    g_object_unref (priv->window_data->texture);

  priv->window_data->texture = texture;
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

  if (priv->sampler != VK_NULL_HANDLE)
    vkDestroySampler (device, priv->sampler, NULL);

  vkCreateSampler (device, &sampler_info, NULL, &priv->sampler);

  xrd_scene_window_update_descriptors (self);

  if (previous_texture_width != extent.width ||
      previous_Texture_height != extent.height)
    {
      /* initial-dims are respective the texture size and ppm.
       * Now that the texture size changed, initial dims need to be
       * updated, using the original ppm used to create this window. */
      float initial_width_meter, initial_height_meter;
      g_object_get (self,
                    "initial-width-meters", &initial_width_meter,
                    "initial-height-meters", &initial_height_meter,
                    NULL);

      float previous_ppm = (float)previous_texture_width / initial_width_meter;
      float new_initial_width_meter = (float) extent.width / previous_ppm;

      /* updates "initial-width-meters"  and "initial height-meters"! */
      xrd_scene_window_set_width_meters (self, new_initial_width_meter);
    }

  xrd_render_unlock ();
}

static GulkanTexture *
_get_texture (XrdWindow *window)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);

  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  return priv->window_data->texture;
}

static void
_poll_event (XrdWindow *self)
{
  (void) self;
}

static void
_add_child (XrdWindow        *window,
            XrdWindow        *child,
            graphene_point_t *offset_center)
{
  (void) window;
  (void) child;
  (void) offset_center;
}

static void
_set_flip_y (XrdWindow *window,
             gboolean   flip_y)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  priv->flip_y = flip_y;
  priv->shading_buffer_data.flip_y = flip_y;

  gulkan_uniform_buffer_update (priv->shading_buffer,
                                (gpointer) &priv->shading_buffer_data);
}

void
xrd_scene_window_set_width_meters (XrdSceneWindow *self,
                                   float           width_meters)
{
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  float height_meters = width_meters / priv->aspect_ratio;

  g_object_set (self,
                "initial-width-meters", (double) width_meters,
                "initial-height-meters", (double) height_meters,
                "scale", 1.0, /* Reset window scale */
                NULL);

  xrd_scene_object_set_scale (XRD_SCENE_OBJECT (self), height_meters);
}

static XrdWindowData*
_get_data (XrdWindow *window)
{
  XrdSceneWindow *self = XRD_SCENE_WINDOW (window);
  XrdSceneWindowPrivate *priv = xrd_scene_window_get_instance_private (self);
  return priv->window_data;
}

static gboolean
_set_sort_order(XrdWindow *window, uint32_t sort_order)
{
  (void)window;
  (void)sort_order;
  return FALSE;
}

static void
xrd_scene_window_window_interface_init (XrdWindowInterface *iface)
{
  iface->set_sort_order = _set_sort_order;
  iface->set_transformation = _set_transformation;
  iface->get_transformation = _get_transformation;
  iface->get_transformation_no_scale = _get_transformation_no_scale;
  iface->submit_texture = _submit_texture;
  iface->set_and_submit_texture = _set_and_submit_texture;
  iface->get_texture = _get_texture;
  iface->poll_event = _poll_event;
  iface->add_child = _add_child;
  iface->set_color = (void (*)(XrdWindow*, const graphene_vec3_t*)) xrd_scene_window_set_color;
  iface->set_flip_y = _set_flip_y;
  iface->show = (void (*)(XrdWindow*)) xrd_scene_object_show;
  iface->hide = (void (*)(XrdWindow*)) xrd_scene_object_hide;
  iface->is_visible = (gboolean (*)(XrdWindow*)) xrd_scene_object_is_visible;
  iface->get_data = _get_data;
}
