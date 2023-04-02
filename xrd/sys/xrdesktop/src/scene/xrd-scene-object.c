/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-object.h"
#include <gulkan.h>

#include "xrd-scene-renderer.h"
#include "graphene-ext.h"

typedef struct _XrdSceneObjectPrivate
{
  GObject parent;

  GulkanClient *gulkan;

  GulkanUniformBuffer *uniform_buffers[2];

  GulkanDescriptorPool *descriptor_pool;
  VkDescriptorSet descriptor_sets[2];

  graphene_matrix_t model_matrix;

  graphene_point3d_t position;
  float scale;
  graphene_quaternion_t orientation;

  gboolean visible;

  gboolean initialized;
} XrdSceneObjectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (XrdSceneObject, xrd_scene_object, G_TYPE_OBJECT)

static void
xrd_scene_object_finalize (GObject *gobject);

static void
xrd_scene_object_class_init (XrdSceneObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_object_finalize;
}

static void
xrd_scene_object_init (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);

  priv->descriptor_pool = NULL;
  graphene_matrix_init_identity (&priv->model_matrix);
  priv->scale = 1.0f;
  priv->visible = TRUE;
  priv->initialized = FALSE;
  priv->gulkan = NULL;
}

static void
xrd_scene_object_finalize (GObject *gobject)
{
  XrdSceneObject *self = XRD_SCENE_OBJECT (gobject);
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  if (!priv->initialized)
    return;

  g_object_unref (priv->descriptor_pool);
  for (uint32_t eye = 0; eye < 2; eye++)
    g_object_unref (priv->uniform_buffers[eye]);

  g_clear_object (&priv->gulkan);

  G_OBJECT_CLASS (xrd_scene_object_parent_class)->finalize (gobject);
}

static void
_update_model_matrix (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_matrix_init_scale (&priv->model_matrix,
                              priv->scale, priv->scale, priv->scale);
  graphene_matrix_rotate_quaternion (&priv->model_matrix, &priv->orientation);
  graphene_matrix_translate (&priv->model_matrix, &priv->position);
}



void
xrd_scene_object_bind (XrdSceneObject    *self,
                       GxrEye             eye,
                       VkCommandBuffer    cmd_buffer,
                       VkPipelineLayout   pipeline_layout)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  vkCmdBindDescriptorSets (
    cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
   &priv->descriptor_sets[eye], 0, NULL);
}

void
xrd_scene_object_set_scale (XrdSceneObject *self, float scale)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  priv->scale = scale;
  _update_model_matrix (self);
}

void
xrd_scene_object_set_position (XrdSceneObject     *self,
                               graphene_point3d_t *position)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_point3d_init_from_point (&priv->position, position);
  _update_model_matrix (self);
}

void
xrd_scene_object_get_position (XrdSceneObject     *self,
                               graphene_point3d_t *position)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_point3d_init_from_point (position, &priv->position);
}

void
xrd_scene_object_set_rotation_euler (XrdSceneObject   *self,
                                     graphene_euler_t *euler)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_quaternion_init_from_euler (&priv->orientation, euler);
  _update_model_matrix (self);
}

gboolean
xrd_scene_object_initialize (XrdSceneObject        *self,
                             GulkanClient          *gulkan,
                             VkDescriptorSetLayout *layout,
                             VkDeviceSize           uniform_buffer_size)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  priv->gulkan = g_object_ref (gulkan);

  GulkanDevice *device = gulkan_client_get_device (gulkan);
  VkDevice vk_device = gulkan_device_get_handle (device);

  /* Create uniform buffer to hold a matrix per eye */
  for (uint32_t eye = 0; eye < 2; eye++)
    {
      priv->uniform_buffers[eye] =
        gulkan_uniform_buffer_new (device, uniform_buffer_size);
      if (!priv->uniform_buffers[eye])
        return FALSE;
    }

  uint32_t set_count = 2;

  VkDescriptorPoolSize pool_sizes[] = {
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    },
    {
      .descriptorCount = set_count,
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    }
  };

  priv->descriptor_pool =
    gulkan_descriptor_pool_new_from_layout (vk_device, *layout, pool_sizes,
                                            G_N_ELEMENTS (pool_sizes),
                                            set_count);
  if (!priv->descriptor_pool)
    return FALSE;

  for (uint32_t eye = 0; eye < set_count; eye++)
    if (!gulkan_descriptor_pool_allocate_sets (priv->descriptor_pool,
                                               1, &priv->descriptor_sets[eye]))
      return FALSE;

  priv->initialized = TRUE;

  return TRUE;
}

void
xrd_scene_object_update_descriptors_texture (XrdSceneObject *self,
                                             VkSampler       sampler,
                                             VkImageView     image_view)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  VkDevice device = gulkan_client_get_device_handle (priv->gulkan);

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = priv->descriptor_sets[eye],
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &(VkDescriptorBufferInfo) {
            .buffer = gulkan_uniform_buffer_get_handle (
                        priv->uniform_buffers[eye]),
            .offset = 0,
            .range = VK_WHOLE_SIZE
          },
          .pTexelBufferView = NULL
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = priv->descriptor_sets[eye],
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &(VkDescriptorImageInfo) {
            .sampler = sampler,
            .imageView = image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
          },
          .pBufferInfo = NULL,
          .pTexelBufferView = NULL
        }
      };

      vkUpdateDescriptorSets (device, 2, write_descriptor_sets, 0, NULL);
    }
}

void
xrd_scene_object_update_descriptors (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  VkDevice device = gulkan_client_get_device_handle (priv->gulkan);

  for (uint32_t eye = 0; eye < 2; eye++)
    {
      VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = priv->descriptor_sets[eye],
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &(VkDescriptorBufferInfo) {
            .buffer = gulkan_uniform_buffer_get_handle (
                        priv->uniform_buffers[eye]),
            .offset = 0,
            .range = VK_WHOLE_SIZE
          },
          .pTexelBufferView = NULL
        }
      };

      vkUpdateDescriptorSets (device, 1, write_descriptor_sets, 0, NULL);
    }
}

void
xrd_scene_object_set_transformation (XrdSceneObject    *self,
                                     graphene_matrix_t *mat)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_ext_matrix_get_rotation_quaternion (mat, &priv->orientation);
  graphene_ext_matrix_get_translation_point3d (mat, &priv->position);

  // graphene_vec3_t scale;
  // graphene_matrix_get_scale (mat, &scale);

  _update_model_matrix (self);
}

/*
 * Set transformation without matrix decomposition and ability to rebuild
 * This will include scale as well.
 */

void
xrd_scene_object_set_transformation_direct (XrdSceneObject    *self,
                                            graphene_matrix_t *mat)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_matrix_init_from_matrix (&priv->model_matrix, mat);
}

void
xrd_scene_object_get_transformation (XrdSceneObject    *self,
                                     graphene_matrix_t *transformation)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  graphene_matrix_init_from_matrix (transformation, &priv->model_matrix);
}

graphene_matrix_t
xrd_scene_object_get_transformation_no_scale (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);

  graphene_matrix_t mat;
  graphene_matrix_init_identity (&mat);
  graphene_matrix_rotate_quaternion (&mat, &priv->orientation);
  graphene_matrix_translate (&mat, &priv->position);
  return mat;
}

gboolean
xrd_scene_object_is_visible (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  return priv->visible;
}

void
xrd_scene_object_show (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  priv->visible = TRUE;
}

void
xrd_scene_object_hide (XrdSceneObject *self)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  priv->visible = FALSE;
}

GulkanUniformBuffer *
xrd_scene_object_get_ubo (XrdSceneObject *self, uint32_t eye)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  return priv->uniform_buffers[eye];
}

VkBuffer
xrd_scene_object_get_transformation_buffer (XrdSceneObject *self, uint32_t eye)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  return gulkan_uniform_buffer_get_handle (priv->uniform_buffers[eye]);
}

VkDescriptorSet
xrd_scene_object_get_descriptor_set (XrdSceneObject *self, uint32_t eye)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  return priv->descriptor_sets[eye];
}

void
xrd_scene_object_update_ubo (XrdSceneObject *self,
                             GxrEye          eye,
                             gpointer        uniform_buffer)
{
  XrdSceneObjectPrivate *priv = xrd_scene_object_get_instance_private (self);
  gulkan_uniform_buffer_update (priv->uniform_buffers[eye], uniform_buffer);
}
