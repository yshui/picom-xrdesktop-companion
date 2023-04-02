/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-context-private.h"
#include "gxr-config.h"
#include "gxr-backend-private.h"
#include "gxr-controller.h"
#include "gxr-device-manager.h"

typedef struct _GxrContextPrivate
{
  GObject parent;
  GulkanClient *gc;
  GxrApi api;

  GxrDeviceManager *device_manager;
} GxrContextPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrContext, gxr_context, G_TYPE_OBJECT)

enum {
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  QUIT_EVENT,
  DEVICE_UPDATE_EVENT,
  BINDING_LOADED,
  BINDINGS_UPDATE,
  ACTIONSET_UPDATE,
  LAST_SIGNAL
};

static guint context_signals[LAST_SIGNAL] = { 0 };

static void
gxr_context_finalize (GObject *gobject);

static void
gxr_context_class_init (GxrContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_context_finalize;

  context_signals[KEYBOARD_PRESS_EVENT] =
  g_signal_new ("keyboard-press-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[KEYBOARD_CLOSE_EVENT] =
  g_signal_new ("keyboard-close-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[QUIT_EVENT] =
  g_signal_new ("quit-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[DEVICE_UPDATE_EVENT] =
  g_signal_new ("device-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  context_signals[BINDINGS_UPDATE] =
  g_signal_new ("bindings-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[BINDING_LOADED] =
  g_signal_new ("binding-loaded-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  context_signals[ACTIONSET_UPDATE] =
  g_signal_new ("action-set-update-event",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static gboolean
_init_runtime (GxrContext *self,
               GxrAppType  type,
               char       *app_name,
               uint32_t    app_version)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_runtime == NULL)
    return FALSE;
  return klass->init_runtime (self, type, app_name, app_version);
}

static gboolean
_init_session (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_session == NULL)
    return FALSE;
  return klass->init_session (self);
}

static void
_add_missing (GSList **target, GSList *source)
{
  for (GSList *l = source; l; l = l->next)
    {
      gchar *source_ext = l->data;
      if (!g_slist_find_custom (*target, source_ext, (GCompareFunc) g_strcmp0))
        *target = g_slist_append (*target, g_strdup (source_ext));
    }
}

static GxrContext *
_new (GxrAppType  type,
      GxrApi      api,
      GSList     *instance_ext_list,
      GSList     *device_ext_list,
      char       *app_name,
      uint32_t    app_version)
{
  GxrBackend *backend = gxr_backend_get_instance (api);
  if (!backend)
    {
      g_print ("Failed to load backend %d\n", api);
      return NULL;
    }

  GxrContext *self = gxr_backend_new_context (backend);
  if (!self)
  {
    g_printerr ("Could not init gxr context.\n");
    return NULL;
  }

  if (!_init_runtime (self, type, app_name, app_version))
    {
      g_object_unref (self);
      g_printerr ("Could not init runtime.\n");
      return NULL;
    }

  GxrContextPrivate *priv = gxr_context_get_instance_private (self);

  GSList *runtime_instance_ext_list = NULL;
  gxr_context_get_instance_extensions (self, &runtime_instance_ext_list);

  /* vk instance required for checking required device exts */
  GulkanClient *gc_tmp =
    gulkan_client_new_from_extensions (runtime_instance_ext_list, NULL);

  if (!gc_tmp)
    {
      g_object_unref (self);
      g_slist_free_full (runtime_instance_ext_list, g_free);
      g_printerr ("Could not init gulkan client from instance exts.\n");
      return NULL;
    }

  GSList *runtime_device_ext_list = NULL;

  gxr_context_get_device_extensions (self, gc_tmp, &runtime_device_ext_list);

  g_object_unref (gc_tmp);

  _add_missing (&runtime_instance_ext_list, instance_ext_list);
  _add_missing (&runtime_device_ext_list, device_ext_list);

  priv->gc =
    gulkan_client_new_from_extensions (runtime_instance_ext_list, runtime_device_ext_list);

  g_slist_free_full (runtime_instance_ext_list, g_free);
  g_slist_free_full (runtime_device_ext_list, g_free);

  if (!priv->gc)
    {
      g_object_unref (self);
      g_printerr ("Could not init gulkan client.\n");
      return NULL;
    }

  if (!_init_session (self))
    {
      g_object_unref (self);
      g_printerr ("Could not init VR session.\n");
      return NULL;
    }

  return self;
}

static GxrApi
_parse_api_from_env ()
{
  const gchar *api_env = g_getenv ("GXR_API");
  if (g_strcmp0 (api_env, "openxr") == 0)
    return GXR_API_OPENXR;
  else if (g_strcmp0 (api_env, "openvr") == 0)
    return GXR_API_OPENVR;
  else
    return GXR_API_NONE;
}

static GxrApi
_get_api_from_env ()
{
  GxrApi parsed_api = _parse_api_from_env ();
  if (parsed_api == GXR_API_NONE)
    return GXR_DEFAULT_API;

  return parsed_api;
}

GxrContext *gxr_context_new (GxrAppType type,
                             char      *app_name,
                             uint32_t   app_version)
{
  return gxr_context_new_from_api (type, _get_api_from_env (),
                                   app_name, app_version);
}

GxrContext *gxr_context_new_from_vulkan_extensions (GxrAppType type,
                                                    GSList *instance_ext_list,
                                                    GSList *device_ext_list,
                                                    char       *app_name,
                                                    uint32_t    app_version)
{
  return gxr_context_new_full (type, _get_api_from_env (),
                               instance_ext_list, device_ext_list,
                               app_name, app_version);
}

GxrContext *gxr_context_new_full (GxrAppType type,
                                  GxrApi     api,
                                  GSList    *instance_ext_list,
                                  GSList    *device_ext_list,
                                  char      *app_name,
                                  uint32_t   app_version)
{
  /* Override with API from env */
  GxrApi api_env = _parse_api_from_env ();
  if (api_env != GXR_API_NONE)
    api = api_env;

  return _new (type, api, instance_ext_list, device_ext_list,
               app_name, app_version);
}

GxrContext *
gxr_context_new_from_api (GxrAppType type,
                          GxrApi     api,
                          char      *app_name,
                          uint32_t   app_version)
{
  return gxr_context_new_full (type, api, NULL, NULL, app_name, app_version);
}

GxrContext *gxr_context_new_headless (char    *app_name,
                                      uint32_t app_version)
{
  GxrApi api = _get_api_from_env ();
  return gxr_context_new_headless_from_api (api, app_name, app_version);
}

GxrContext *gxr_context_new_headless_from_api (GxrApi   api,
                                               char    *app_name,
                                               uint32_t app_version)
{
  /* Override with API from env */
  GxrApi api_env = _parse_api_from_env ();
  if (api_env != GXR_API_NONE)
    api = api_env;

  GxrBackend *backend = gxr_backend_get_instance (api);
  if (!backend)
    {
      g_print ("Failed to load backend %d\n", api);
      return NULL;
    }

  GxrContext *self = gxr_backend_new_context (backend);
  if (!_init_runtime (self, GXR_APP_HEADLESS, app_name, app_version))
    {
      g_print ("Could not init VR runtime.\n");
      return NULL;
    }
  return self;
}

static void
gxr_context_init (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  priv->api = GXR_API_NONE;
  priv->gc = NULL;
  priv->device_manager = gxr_device_manager_new ();
}

static void
gxr_context_finalize (GObject *gobject)
{
  GxrContext *self = GXR_CONTEXT (gobject);
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);

  if (priv->device_manager != NULL)
    g_clear_object (&priv->device_manager);

  /* child classes MUST destroy gulkan after this destructor finishes */

  G_OBJECT_CLASS (gxr_context_parent_class)->finalize (gobject);
}

GxrApi
gxr_context_get_api (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  return priv->api;
}

GulkanClient*
gxr_context_get_gulkan (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  return priv->gc;
}

void
gxr_context_set_api (GxrContext *self, GxrApi api)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  priv->api = api;
}

gboolean
gxr_context_get_head_pose (GxrContext *self, graphene_matrix_t *pose)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_head_pose == NULL)
    return FALSE;
  return klass->get_head_pose (self, pose);
}

void
gxr_context_get_frustum_angles (GxrContext *self, GxrEye eye,
                                float *left, float *right,
                                float *top, float *bottom)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_frustum_angles == NULL)
    return;
  klass->get_frustum_angles (self, eye, left, right, top, bottom);
}

gboolean
gxr_context_is_input_available (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_input_available == NULL)
    return FALSE;
  return klass->is_input_available (self);
}

void
gxr_context_get_render_dimensions (GxrContext *self,
                                   VkExtent2D *extent)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_render_dimensions == NULL)
      return;
  klass->get_render_dimensions (self, extent);
}

void
gxr_context_poll_event (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->poll_event == NULL)
    return;
  klass->poll_event (self);
}

void
gxr_context_show_keyboard (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->show_keyboard == NULL)
    return;
  klass->show_keyboard (self);
}

void
gxr_context_emit_keyboard_press (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[KEYBOARD_PRESS_EVENT], 0, event);
}

void
gxr_context_emit_keyboard_close (GxrContext *self)
{
  g_signal_emit (self, context_signals[KEYBOARD_CLOSE_EVENT], 0);
}

void
gxr_context_emit_quit (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[QUIT_EVENT], 0, event);
}


void
gxr_context_emit_device_update (GxrContext *self, gpointer event)
{
  g_signal_emit (self, context_signals[DEVICE_UPDATE_EVENT], 0, event);
}

void
gxr_context_emit_bindings_update (GxrContext *self)
{
  g_signal_emit (self, context_signals[BINDINGS_UPDATE], 0);
}

void
gxr_context_emit_binding_loaded (GxrContext *self)
{
  g_signal_emit (self, context_signals[BINDING_LOADED], 0);
}

void
gxr_context_emit_actionset_update (GxrContext *self)
{
  g_signal_emit (self, context_signals[ACTIONSET_UPDATE], 0);
}

gboolean
gxr_context_init_framebuffers (GxrContext           *self,
                               VkExtent2D            extent,
                               VkSampleCountFlagBits sample_count,
                               GulkanRenderPass    **render_pass)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->init_framebuffers == NULL)
    return FALSE;
  return klass->init_framebuffers (self, extent, sample_count, render_pass);
}

gboolean
gxr_context_submit_framebuffers (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->submit_framebuffers == NULL)
    return FALSE;
  return klass->submit_framebuffers (self);
}

uint32_t
gxr_context_get_model_vertex_stride (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_vertex_stride == NULL)
    return 0;
  return klass->get_model_vertex_stride (self);
}

uint32_t
gxr_context_get_model_normal_offset (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_normal_offset == NULL)
    return 0;
  return klass->get_model_normal_offset (self);
}

uint32_t
gxr_context_get_model_uv_offset (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_uv_offset == NULL)
    return 0;
  return klass->get_model_uv_offset (self);
}

void
gxr_context_get_projection (GxrContext *self,
                            GxrEye eye,
                            float near,
                            float far,
                            graphene_matrix_t *mat)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_projection == NULL)
    return;
  klass->get_projection (self, eye, near, far, mat);
}

void
gxr_context_get_view (GxrContext *self,
                      GxrEye eye,
                      graphene_matrix_t *mat)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_view == NULL)
    return;
  klass->get_view (self, eye, mat);
}

gboolean
gxr_context_begin_frame (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->begin_frame == NULL)
    return FALSE;

  GxrPose poses[GXR_DEVICE_INDEX_MAX];

  gboolean res = klass->begin_frame (self, poses);

  GxrDeviceManager *dm = gxr_context_get_device_manager (self);
  gxr_device_manager_update_poses (dm, poses);

  return res;
}

gboolean
gxr_context_end_frame (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->end_frame == NULL)
    return FALSE;

  gboolean res = klass->end_frame (self);

  return res;
}

GxrActionSet *
gxr_context_new_action_set_from_url (GxrContext *self, gchar *url)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->new_action_set_from_url == NULL)
    return FALSE;
  return klass->new_action_set_from_url (self, url);
}

gboolean
gxr_context_load_action_manifest (GxrContext *self,
                                  const char *cache_name,
                                  const char *resource_path,
                                  const char *manifest_name)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->load_action_manifest == NULL)
    return FALSE;

  gboolean ret = klass->load_action_manifest (self,
                                              cache_name,
                                              resource_path,
                                              manifest_name);

  return ret;
}

void
gxr_context_acknowledge_quit (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->acknowledge_quit == NULL)
    return;
  klass->acknowledge_quit (self);
}

gboolean
gxr_context_is_tracked_device_connected (GxrContext *self, uint32_t i)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_tracked_device_connected == NULL)
    return FALSE;
  return klass->is_tracked_device_connected (self, i);
}

gboolean
gxr_context_device_is_controller (GxrContext *self, uint32_t i)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->device_is_controller == NULL)
    return FALSE;
  return klass->device_is_controller (self, i);
}

gchar*
gxr_context_get_device_model_name (GxrContext *self, uint32_t i)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_device_model_name == NULL)
    return NULL;
  return klass->get_device_model_name (self, i);
}

gboolean
gxr_context_load_model (GxrContext         *self,
                        GulkanVertexBuffer *vbo,
                        GulkanTexture     **texture,
                        VkSampler          *sampler,
                        const char         *model_name)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->load_model == NULL)
    return FALSE;
  return klass->load_model (self, vbo, texture, sampler, model_name);
}

gboolean
gxr_context_is_another_scene_running (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->is_another_scene_running  == NULL)
    return FALSE;
  return klass->is_another_scene_running (self);
}

void
gxr_context_request_quit (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->request_quit  == NULL)
    return;
  klass->request_quit (self);
}

void
gxr_context_set_keyboard_transform (GxrContext        *self,
                                    graphene_matrix_t *transform)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->set_keyboard_transform  == NULL)
    return;
  klass->set_keyboard_transform (self, transform);
}

GxrAction *
gxr_context_new_action_from_type_url (GxrContext   *self,
                                      GxrActionSet *action_set,
                                      GxrActionType type,
                                      char          *url)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->new_action_from_type_url == NULL)
    return FALSE;
  return klass->new_action_from_type_url (self, action_set, type, url);
}

GxrOverlay *
gxr_context_new_overlay (GxrContext *self, gchar* key)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->new_overlay == NULL)
    return FALSE;
  return klass->new_overlay (self, key);
}

GSList *
gxr_context_get_model_list (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_model_list == NULL)
    return FALSE;
  return klass->get_model_list (self);
}


gboolean
gxr_context_get_instance_extensions (GxrContext *self, GSList **out_list)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_instance_extensions == NULL)
    return FALSE;
  return klass->get_instance_extensions (self, out_list);
}

gboolean
gxr_context_get_device_extensions (GxrContext   *self,
                                   GulkanClient *gc,
                                   GSList      **out_list)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_device_extensions == NULL)
    return FALSE;
  return klass->get_device_extensions (self, gc, out_list);
}

GxrDeviceManager *
gxr_context_get_device_manager (GxrContext *self)
{
  GxrContextPrivate *priv = gxr_context_get_instance_private (self);
  return priv->device_manager;
}

uint32_t
gxr_context_get_view_count (GxrContext *self)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_view_count == NULL)
    return FALSE;
  return klass->get_view_count (self);
}

GulkanFrameBuffer *
gxr_context_get_acquired_framebuffer (GxrContext *self, uint32_t view)
{
  GxrContextClass *klass = GXR_CONTEXT_GET_CLASS (self);
  if (klass->get_acquired_framebuffer == NULL)
    return FALSE;
  return klass->get_acquired_framebuffer (self, view);
}
