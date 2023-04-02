/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-descriptor-pool.h"
#include "gulkan-client.h"

struct _GulkanDescriptorPool
{
  GObject parent;

  VkDevice device;

  VkDescriptorPool handle;
  VkDescriptorSetLayout set_layout;
  VkPipelineLayout pipeline_layout;

  gboolean free_layouts;
};

G_DEFINE_TYPE (GulkanDescriptorPool, gulkan_descriptor_pool, G_TYPE_OBJECT)

static void
gulkan_descriptor_pool_init (GulkanDescriptorPool *self)
{
  self->device = VK_NULL_HANDLE;
  self->handle = VK_NULL_HANDLE;
  self->set_layout = VK_NULL_HANDLE;
  self->pipeline_layout = VK_NULL_HANDLE;
  self->free_layouts = FALSE;
}

static void
_finalize (GObject *gobject)
{
  GulkanDescriptorPool *self = GULKAN_DESCRIPTOR_POOL (gobject);
  if (self->handle != VK_NULL_HANDLE)
    vkDestroyDescriptorPool (self->device, self->handle, NULL);
  if (self->free_layouts)
    {
      if (self->set_layout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout (self->device, self->set_layout, NULL);
      if (self->pipeline_layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout (self->device, self->pipeline_layout, NULL);
    }
}

static void
gulkan_descriptor_pool_class_init (GulkanDescriptorPoolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

static gboolean
_init_layout (GulkanDescriptorPool *self,
              const VkDescriptorSetLayoutBinding *bindings,
              uint32_t binding_count)
{
  VkDescriptorSetLayoutCreateInfo descriptor_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = binding_count,
    .pBindings = bindings,
  };

  VkResult res;
  res = vkCreateDescriptorSetLayout (self->device, &descriptor_info,
                                     NULL, &self->set_layout);
  vk_check_error ("vkCreateDescriptorSetLayout", res, FALSE);

  VkPipelineLayoutCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &self->set_layout
  };

  res = vkCreatePipelineLayout (self->device, &pipeline_info,
                                NULL, &self->pipeline_layout);
  vk_check_error ("vkCreatePipelineLayout", res, FALSE);

  self->free_layouts = TRUE;

  return TRUE;
}

static gboolean
_init (GulkanDescriptorPool       *self,
       const VkDescriptorPoolSize *pool_sizes,
       uint32_t                    pool_size_count,
       uint32_t                    set_count)
{
  VkDescriptorPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = set_count,
    .poolSizeCount = pool_size_count,
    .pPoolSizes = pool_sizes
  };

  VkResult res = vkCreateDescriptorPool (self->device, &info, NULL,
                                         &self->handle);
  vk_check_error ("vkCreateDescriptorPool", res, FALSE);

  return TRUE;
}

GulkanDescriptorPool *
gulkan_descriptor_pool_new_from_layout (VkDevice                    device,
                                        VkDescriptorSetLayout       layout,
                                        const VkDescriptorPoolSize *pool_sizes,
                                        uint32_t                    pool_size_count,
                                        uint32_t                    set_count)
{
  GulkanDescriptorPool* self =
    (GulkanDescriptorPool*) g_object_new (GULKAN_TYPE_DESCRIPTOR_POOL, 0);
  self->device = device;
  self->set_layout = layout;

  if (!_init (self, pool_sizes, pool_size_count, set_count))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

GulkanDescriptorPool *
gulkan_descriptor_pool_new (VkDevice                            device,
                            const VkDescriptorSetLayoutBinding *bindings,
                            uint32_t                            binding_count,
                            const VkDescriptorPoolSize         *pool_sizes,
                            uint32_t                            pool_size_count,
                            uint32_t                            set_count)
{
  GulkanDescriptorPool *self =
    gulkan_descriptor_pool_new_from_layout (device, VK_NULL_HANDLE, pool_sizes,
                                            pool_size_count, set_count);

  if (!self)
    return NULL;

  if (!_init_layout (self, bindings, binding_count))
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

gboolean
gulkan_descriptor_pool_allocate_sets (GulkanDescriptorPool *self,
                                      uint32_t              count,
                                      VkDescriptorSet      *sets)
{
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = self->handle,
    .descriptorSetCount = count,
    .pSetLayouts = &self->set_layout
  };

  VkResult res = vkAllocateDescriptorSets (self->device, &alloc_info, sets);
  vk_check_error ("vkAllocateDescriptorSets", res, FALSE);

  return TRUE;
}

VkPipelineLayout
gulkan_descriptor_pool_get_pipeline_layout (GulkanDescriptorPool *self)
{
  return self->pipeline_layout;
}
