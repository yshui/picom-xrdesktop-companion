/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTEXT_H_
#define GXR_CONTEXT_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>
#include <stdint.h>
#include <gulkan.h>

#include "gxr-enums.h"
#include "gxr-types.h"
#include "gxr-action-set.h"
#include "gxr-overlay.h"
#include "gxr-device-manager.h"

G_BEGIN_DECLS

#define GXR_TYPE_CONTEXT gxr_context_get_type()
G_DECLARE_DERIVABLE_TYPE (GxrContext, gxr_context, GXR, CONTEXT, GObject)

struct _GxrContextClass
{
  GObjectClass parent;

  gboolean
  (*get_head_pose) (GxrContext *self, graphene_matrix_t *pose);

  void
  (*get_frustum_angles) (GxrContext *self, GxrEye eye,
                         float *left, float *right,
                         float *top, float *bottom);

  gboolean
  (*is_input_available) (GxrContext *self);

  void
  (*get_render_dimensions) (GxrContext *context,
                            VkExtent2D *extent);

  void
  (*poll_event) (GxrContext *self);

  void
  (*show_keyboard) (GxrContext *self);

  gboolean
  (*init_runtime) (GxrContext *self,
                   GxrAppType  type,
                   char       *app_name,
                   uint32_t    app_version);

  gboolean
  (*init_session) (GxrContext *self);

  gboolean
  (*init_framebuffers) (GxrContext           *self,
                        VkExtent2D            extent,
                        VkSampleCountFlagBits sample_count,
                        GulkanRenderPass    **render_pass);

  gboolean
  (*submit_framebuffers) (GxrContext *self);

  uint32_t
  (*get_model_vertex_stride) (GxrContext *self);

  uint32_t
  (*get_model_normal_offset) (GxrContext *self);

  uint32_t
  (*get_model_uv_offset) (GxrContext *self);

  void
  (*get_projection) (GxrContext *self,
                     GxrEye eye,
                     float near,
                     float far,
                     graphene_matrix_t *mat);

  void
  (*get_view) (GxrContext *self,
               GxrEye eye,
               graphene_matrix_t *mat);

  gboolean
  (*begin_frame) (GxrContext *self,
                  GxrPose    *poses);

  gboolean
  (*end_frame) (GxrContext *self);

  void
  (*acknowledge_quit) (GxrContext *self);

  gboolean
  (*is_tracked_device_connected) (GxrContext *self, uint32_t i);

  gboolean
  (*device_is_controller) (GxrContext *self, uint32_t i);

  gchar*
  (*get_device_model_name) (GxrContext *self, uint32_t i);

  gboolean
  (*load_model) (GxrContext         *self,
                 GulkanVertexBuffer *vbo,
                 GulkanTexture     **texture,
                 VkSampler          *sampler,
                 const char         *model_name);

  gboolean
  (*is_another_scene_running) (GxrContext *self);

  void
  (*set_keyboard_transform) (GxrContext        *self,
                             graphene_matrix_t *transform);

  GSList *
  (*get_model_list) (GxrContext *self);

  GxrActionSet *
  (*new_action_set_from_url) (GxrContext *self, gchar *url);

  gboolean
  (*load_action_manifest) (GxrContext *self,
                           const char *cache_name,
                           const char *resource_path,
                           const char *manifest_name);

  GxrAction *
  (*new_action_from_type_url) (GxrContext   *context,
                               GxrActionSet *action_set,
                               GxrActionType type,
                               char          *url);

  GxrOverlay *
  (*new_overlay) (GxrContext *self, gchar* key);

  void
  (*request_quit) (GxrContext *self);

  gboolean
  (*get_instance_extensions) (GxrContext *self, GSList **out_list);

  gboolean
  (*get_device_extensions) (GxrContext   *self,
                            GulkanClient *gc,
                            GSList      **out_list);

  uint32_t
  (*get_view_count) (GxrContext *self);

  GulkanFrameBuffer *
  (*get_acquired_framebuffer) (GxrContext *self, uint32_t view);
};

GxrContext *gxr_context_new (GxrAppType  type,
                             char       *app_name,
                             uint32_t    app_version);

GxrContext *
gxr_context_new_from_api (GxrAppType type,
                          GxrApi     backend,
                          char      *app_name,
                          uint32_t   app_version);

GxrContext *gxr_context_new_from_vulkan_extensions (GxrAppType type,
                                                    GSList *instance_ext_list,
                                                    GSList *device_ext_list,
                                                    char       *app_name,
                                                    uint32_t    app_version);

GxrContext *gxr_context_new_headless (char    *app_name,
                                      uint32_t app_version);

GxrContext *gxr_context_new_headless_from_api (GxrApi   api,
                                               char    *app_name,
                                               uint32_t app_version);

GxrContext *gxr_context_new_full (GxrAppType type,
                                  GxrApi     api,
                                  GSList    *instance_ext_list,
                                  GSList    *device_ext_list,
                                  char       *app_name,
                                  uint32_t    app_version);

GxrApi
gxr_context_get_api (GxrContext *self);

gboolean
gxr_context_get_head_pose (GxrContext *self, graphene_matrix_t *pose);

void
gxr_context_get_frustum_angles (GxrContext *self, GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom);

gboolean
gxr_context_is_input_available (GxrContext *self);

void
gxr_context_get_render_dimensions (GxrContext *self,
                                   VkExtent2D *extent);

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               VkExtent2D            extent,
                               VkSampleCountFlagBits sample_count,
                               GulkanRenderPass    **render_pass);

gboolean
gxr_context_submit_framebuffers (GxrContext           *self);

void
gxr_context_poll_event (GxrContext *self);

void
gxr_context_show_keyboard (GxrContext *self);

uint32_t
gxr_context_get_model_vertex_stride (GxrContext *self);

uint32_t
gxr_context_get_model_normal_offset (GxrContext *self);

uint32_t
gxr_context_get_model_uv_offset (GxrContext *self);

void
gxr_context_get_projection (GxrContext *self,
                            GxrEye eye,
                            float near,
                            float far,
                            graphene_matrix_t *mat);

void
gxr_context_get_view (GxrContext *self,
                      GxrEye eye,
                      graphene_matrix_t *mat);

gboolean
gxr_context_begin_frame (GxrContext *self);

gboolean
gxr_context_end_frame (GxrContext *self);

void
gxr_context_acknowledge_quit (GxrContext *self);

gboolean
gxr_context_is_tracked_device_connected (GxrContext *self, uint32_t i);

gboolean
gxr_context_device_is_controller (GxrContext *self, uint32_t i);

gchar* gxr_context_get_device_model_name(GxrContext* self, uint32_t i);

gboolean
gxr_context_load_model (GxrContext         *self,
                        GulkanVertexBuffer *vbo,
                        GulkanTexture     **texture,
                        VkSampler          *sampler,
                        const char         *model_name);

gboolean
gxr_context_is_another_scene_running (GxrContext *self);

void
gxr_context_set_keyboard_transform (GxrContext        *self,
                                    graphene_matrix_t *transform);

GSList *
gxr_context_get_model_list (GxrContext *self);

gboolean
gxr_context_load_action_manifest (GxrContext *self,
                                  const char *cache_name,
                                  const char *resource_path,
                                  const char *manifest_name);

void
gxr_context_request_quit (GxrContext *self);

GulkanClient*
gxr_context_get_gulkan (GxrContext *self);

gboolean
gxr_context_get_instance_extensions (GxrContext *self, GSList **out_list);

gboolean
gxr_context_get_device_extensions (GxrContext   *self,
                                   GulkanClient *gc,
                                   GSList      **out_list);

struct _GxrDeviceManager;

struct _GxrDeviceManager *
gxr_context_get_device_manager (GxrContext *self);

uint32_t
gxr_context_get_view_count (GxrContext *self);

GulkanFrameBuffer *
gxr_context_get_acquired_framebuffer (GxrContext *self, uint32_t view);

G_END_DECLS

#endif /* GXR_CONTEXT_H_ */
