/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_GEOMETRY_H_
#define GULKAN_GEOMETRY_H_

#if !defined (GULKAN_INSIDE) && !defined (GULKAN_COMPILATION)
#error "Only <gulkan.h> can be included directly."
#endif

#include <glib-object.h>

#include "gulkan-vertex-buffer.h"

void
gulkan_geometry_append_axes (GulkanVertexBuffer *self,
                             graphene_vec4_t    *center,
                             float               length,
                             graphene_matrix_t  *mat);

void
gulkan_geometry_append_ray (GulkanVertexBuffer *self,
                            graphene_vec4_t    *center,
                            float               length,
                            graphene_matrix_t  *mat);

void
gulkan_geometry_append_plane (GulkanVertexBuffer *self,
                              graphene_point_t   *from,
                              graphene_point_t   *to,
                              graphene_matrix_t  *mat);

#endif /* GULKAN_GEOMETRY_H_ */
