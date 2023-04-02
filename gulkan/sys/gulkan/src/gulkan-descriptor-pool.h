/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_DESCRIPTOR_POOL_H_
#define GULKAN_DESCRIPTOR_POOL_H_

#include <glib-object.h>
#include <vulkan/vulkan.h>

G_BEGIN_DECLS

#define GULKAN_TYPE_DESCRIPTOR_POOL gulkan_descriptor_pool_get_type()
G_DECLARE_FINAL_TYPE (GulkanDescriptorPool, gulkan_descriptor_pool,
                      GULKAN, DESCRIPTOR_POOL, GObject)

#define GULKAN_DESCRIPTOR_POOL_NEW(a, b, c, d) \
  gulkan_descriptor_pool_new (a, b, G_N_ELEMENTS (b), c, G_N_ELEMENTS (c), d)

GulkanDescriptorPool *
gulkan_descriptor_pool_new_from_layout (VkDevice                    device,
                                        VkDescriptorSetLayout       layout,
                                        const VkDescriptorPoolSize *pool_sizes,
                                        uint32_t                    pool_size_count,
                                        uint32_t                    set_count);

GulkanDescriptorPool *
gulkan_descriptor_pool_new (VkDevice                            device,
                            const VkDescriptorSetLayoutBinding *bindings,
                            uint32_t                            binding_count,
                            const VkDescriptorPoolSize         *pool_sizes,
                            uint32_t                            pool_size_count,
                            uint32_t                            set_count);

gboolean
gulkan_descriptor_pool_allocate_sets (GulkanDescriptorPool *self,
                                      uint32_t              count,
                                      VkDescriptorSet      *set);

VkPipelineLayout
gulkan_descriptor_pool_get_pipeline_layout (GulkanDescriptorPool *self);

G_END_DECLS

#endif /* GULKAN_DESCRIPTOR_POOL_H_ */
