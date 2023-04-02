/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-client-private.h"

#include "xrd-client-private.h"

#include "xrd-scene-window-private.h"
#include "xrd-scene-desktop-cursor.h"
#include "xrd-scene-pointer-tip.h"
#include "xrd-scene-renderer.h"
#include "xrd-scene-background.h"
#include "xrd-settings.h"
#include "xrd-render-lock.h"
#include "xrd-scene-pointer.h"
#include "xrd-scene-model.h"
#include "xrd-scene-selection.h"

struct _XrdSceneClient
{
  XrdClient parent;

  graphene_matrix_t mat_view[2];
  graphene_matrix_t mat_projection[2];

  float near;
  float far;

  gboolean have_projection;

  XrdSceneRenderer *renderer;
  XrdSceneBackground *background;

  /* Multithreading scene */
  GThread *render_thread;
  volatile gint shutdown_render_thread;
};

G_DEFINE_TYPE (XrdSceneClient, xrd_scene_client, XRD_TYPE_CLIENT)

static void
_scene_device_activate_cb (GxrDeviceManager *device_manager,
                           GxrDevice        *device,
                           gpointer          _self);
static void
_scene_device_deactivate_cb (GxrDeviceManager *device_manager,
                             GxrDevice        *device,
                             gpointer          _self);
static gboolean
_init_vulkan (XrdSceneClient   *self,
              GulkanClient     *gc);

static void*
_render_thread (gpointer _self);

static void
xrd_scene_client_init (XrdSceneClient *self)
{
  xrd_render_lock_init ();
  self->render_thread = NULL;
  self->shutdown_render_thread = 0;

  xrd_client_set_upload_layout (XRD_CLIENT (self),
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  self->near = 0.05f;
  self->far = 100.0f;

  self->have_projection = FALSE;

  self->background = NULL;
  self->renderer = NULL;
}

XrdSceneClient *
xrd_scene_client_new (GxrContext *context)
{
  XrdSceneClient *self =
    (XrdSceneClient *) g_object_new (XRD_TYPE_SCENE_CLIENT, 0);

  xrd_client_set_context (XRD_CLIENT (self), context);

  GulkanClient *gc = gxr_context_get_gulkan (context);

  self->renderer = xrd_scene_renderer_new ();
  if (!_init_vulkan (self, gc))
    {
      g_object_unref (self);
      g_printerr ("Could not init Vulkan.\n");
      return NULL;
    }

  GxrDeviceManager *dm = gxr_context_get_device_manager (context);
  g_signal_connect (dm, "device-activate-event",
                    (GCallback) _scene_device_activate_cb, self);
  g_signal_connect (dm, "device-deactivate-event",
                    (GCallback) _scene_device_deactivate_cb, self);

  xrd_client_init_input_callbacks (XRD_CLIENT (self));

  GError *error = NULL;
  self->render_thread = g_thread_try_new ("xrd-scene-client-render",
                                          (GThreadFunc) _render_thread,
                                          self,
                                          &error);
  if (error != NULL)
    {
      g_printerr ("Unable to start render thread: %s\n", error->message);
      g_error_free (error);
    }

  return self;
}

static void*
_render_thread (gpointer _self)
{
  XrdSceneClient *self = (XrdSceneClient*) _self;

  g_debug ("Starting XrdSceneClient render thread.\n");

  while (!g_atomic_int_get (&self->shutdown_render_thread))
    {
      xrd_scene_client_render (self);
      g_thread_yield ();
    }

  g_debug ("Stopped XrdSceneClient render thread, bye.\n");

  return NULL;
}

static void
_destroy_selections (XrdSceneClient *self)
{
  XrdClient *client = XRD_CLIENT (self);
  GxrContext *context = xrd_client_get_gxr_context (client);
  GxrDeviceManager *dm = gxr_context_get_device_manager (context);

  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);

      gpointer selection = gxr_controller_get_user_data (controller);
      if (selection != NULL)
        {
          XrdSceneSelection *scene_selection = XRD_SCENE_SELECTION (selection);
          gxr_controller_set_user_data (controller, NULL);
          g_object_unref (scene_selection);
        }
    }
}

static void
xrd_scene_client_finalize (GObject *gobject)
{
  XrdSceneClient *self = XRD_SCENE_CLIENT (gobject);

  if (self->render_thread)
    {
      g_atomic_int_set (&self->shutdown_render_thread, 1);
      g_thread_join (self->render_thread);
      self->render_thread = NULL;
      g_atomic_int_set (&self->shutdown_render_thread, 0);
    }

  g_object_unref (self->background);

  _destroy_selections (self);

  xrd_render_lock_destroy ();

  g_clear_object (&self->renderer);

  G_OBJECT_CLASS (xrd_scene_client_parent_class)->finalize (gobject);
}

static gboolean
_init_model (XrdSceneClient *self, GxrDevice *device)
{
  gchar *model_name = gxr_device_get_model_name (device);
  if (!model_name)
  {
    g_printerr ("No model for device\n");
    return FALSE;
  }

  VkDescriptorSetLayout *descriptor_set_layout =
  xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  GulkanClient *gulkan = xrd_scene_renderer_get_gulkan (self->renderer);
  XrdSceneModel *sm = xrd_scene_model_new (descriptor_set_layout, gulkan);
  // g_print ("Loading model %s\n", model_name);
  GxrContext *context = xrd_client_get_gxr_context (XRD_CLIENT (self));
  xrd_scene_model_load (sm, context, model_name);

  gxr_device_set_model (device, GXR_MODEL (sm));
  return TRUE;
}

static void
_scene_device_activate_cb (GxrDeviceManager *device_manager,
                           GxrDevice        *device,
                           gpointer          _self)
{
  (void) device_manager;
  XrdSceneClient *self = XRD_SCENE_CLIENT (_self);

  xrd_render_lock ();

  _init_model (self, device);

  xrd_render_unlock ();

  if (!gxr_device_is_controller (device))
    return;

  GxrController *controller = GXR_CONTROLLER (device);
  g_debug ("scene-client: Controller %lu activated.\n",
           gxr_device_get_handle (GXR_DEVICE (controller)));
}

static void
_scene_device_deactivate_cb (GxrDeviceManager *device_manager,
                             GxrDevice        *device,
                             gpointer          _self)
{
  (void) device_manager;
  (void) _self;

  if (!gxr_device_is_controller (device))
    return;

  GxrController *controller = GXR_CONTROLLER (device);

  XrdSceneSelection *selection =
    XRD_SCENE_SELECTION (gxr_controller_get_user_data (controller));
  g_object_unref (selection);

  g_debug ("scene-client: Controller %lu deactivated.\n",
           gxr_device_get_handle (GXR_DEVICE (controller)));
}

static void
_render_pointers (XrdSceneClient    *self,
                  GxrEye             eye,
                  VkCommandBuffer    cmd_buffer,
                  VkPipeline        *pipelines,
                  VkPipelineLayout   pipeline_layout,
                  graphene_matrix_t *vp)
{
  GxrContext *context = xrd_client_get_gxr_context (XRD_CLIENT (self));
  if (!gxr_context_is_input_available (context))
    return;

  GSList *controllers = xrd_client_get_controllers (XRD_CLIENT (self));
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);

      if (!gxr_controller_is_pointer_pose_valid (controller))
        continue;

      XrdScenePointer *pointer =
        XRD_SCENE_POINTER (gxr_controller_get_pointer (controller));
      xrd_scene_pointer_render (pointer, eye,
                                pipelines[PIPELINE_POINTER],
                                pipeline_layout, cmd_buffer, vp);

      XrdScenePointerTip *scene_tip =
      XRD_SCENE_POINTER_TIP (gxr_controller_get_pointer_tip (controller));
      xrd_scene_pointer_tip_render (scene_tip, eye, pipelines[PIPELINE_TIP],
                                    pipeline_layout, cmd_buffer, vp);
    }
}

static void
_render_selections (XrdSceneClient    *self,
                    GxrEye             eye,
                    VkCommandBuffer    cmd_buffer,
                    VkPipeline        *pipelines,
                    VkPipelineLayout   pipeline_layout,
                    graphene_matrix_t *vp)
{
  GxrContext *context = xrd_client_get_gxr_context (XRD_CLIENT (self));
  if (!gxr_context_is_input_available (context))
    return;

  GSList *controllers = xrd_client_get_controllers (XRD_CLIENT (self));
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);

      gpointer selection = gxr_controller_get_user_data (controller);
      if (selection == NULL)
        continue;

      XrdSceneSelection *scene_selection = XRD_SCENE_SELECTION (selection);

      if (gxr_controller_get_hover_state (controller)->hovered_object != NULL)
        {
          XrdWindow *window =
            XRD_WINDOW (
              gxr_controller_get_hover_state (controller)->hovered_object);

          XrdSceneWindow *scene_window = XRD_SCENE_WINDOW (window);
          XrdSceneObject *window_obj = XRD_SCENE_OBJECT (scene_window);

          graphene_matrix_t window_transformation;
          xrd_scene_object_get_transformation (window_obj,
                                               &window_transformation);


          XrdSceneObject *selection_obj = XRD_SCENE_OBJECT (scene_selection);
          xrd_scene_object_set_transformation_direct (selection_obj,
                                                      &window_transformation);

          float aspect = xrd_window_get_aspect_ratio (window);
          xrd_scene_selection_set_aspect_ratio (scene_selection, aspect);
          xrd_scene_object_show (selection_obj);
        }
      else
      {
        /* g_debug ("Not hovering window, not rendering selection\n"); */
        XrdSceneObject *selection_obj = XRD_SCENE_OBJECT (scene_selection);
        xrd_scene_object_hide (selection_obj);
        return;
      }

      xrd_scene_selection_render (scene_selection, eye,
                                  pipelines[PIPELINE_SELECTION],
                                  pipeline_layout, cmd_buffer, vp);
    }
}

/*
 * Since we are using world space positons for the lights, this only needs
 * to be run once for both eyes
 */
static void
_update_lights_cb (gpointer _self)
{
  XrdSceneClient *self = XRD_SCENE_CLIENT (_self);

  GSList *controllers = xrd_client_get_controllers (XRD_CLIENT (self));

  xrd_scene_renderer_update_lights (self->renderer, controllers);
}

static XrdSceneModel *
_get_scene_model (GxrDevice *device)
{
  GxrModel *model = gxr_device_get_model (device);
  if (model)
    {
      // g_print ("Using model %s\n", gxr_model_get_name (model));
      return XRD_SCENE_MODEL (model);
    }

  return NULL;
}

static void
_render_device (GxrDevice         *device,
                uint32_t           eye,
                VkCommandBuffer    cmd_buffer,
                VkPipelineLayout   pipeline_layout,
                VkPipeline         pipeline,
                graphene_matrix_t *vp,
                XrdSceneClient    *self)
{
  (void) self;
  XrdSceneModel *model = _get_scene_model (device);
  if (!model)
    {
      // g_print ("Device has no model\n");
      return;
    }

  if (!gxr_device_is_pose_valid (device))
    {
      // g_print ("Pose invalid for %s\n", gxr_device_get_model_name (device));
      return;
    }

  graphene_matrix_t transformation;
  gxr_device_get_transformation_direct (device, &transformation);

  xrd_scene_model_render (model, eye, pipeline, cmd_buffer, pipeline_layout,
                          &transformation, vp);
}

static void
_render_devices (uint32_t           eye,
                 VkCommandBuffer    cmd_buffer,
                 VkPipelineLayout   pipeline_layout,
                 VkPipeline        *pipelines,
                 graphene_matrix_t *vp,
                 XrdSceneClient    *self)
{
  GxrContext *context = xrd_client_get_gxr_context (XRD_CLIENT (self));
  GxrDeviceManager *dm = gxr_context_get_device_manager (context);
  GList *devices = gxr_device_manager_get_devices (dm);

  for (GList *l = devices; l; l = l->next)
    _render_device (l->data, eye, cmd_buffer, pipeline_layout,
                    pipelines[PIPELINE_DEVICE_MODELS], vp, self);
  g_list_free (devices);
}

static void
_render_eye_cb (uint32_t         eye,
                VkCommandBuffer  cmd_buffer,
                VkPipelineLayout pipeline_layout,
                VkPipeline      *pipelines,
                gpointer         _self)
{
  XrdSceneClient *self = XRD_SCENE_CLIENT (_self);

  graphene_matrix_t vp;
  graphene_matrix_multiply (&self->mat_view[eye],
                            &self->mat_projection[eye], &vp);

  XrdWindowManager *manager = xrd_client_get_manager (XRD_CLIENT (self));

  xrd_scene_background_render (self->background, eye,
                               pipelines[PIPELINE_BACKGROUND],
                               pipeline_layout, cmd_buffer, &vp);

  for (GSList *l = xrd_window_manager_get_windows (manager);
       l != NULL; l = l->next)
    {
      xrd_scene_window_render (XRD_SCENE_WINDOW (l->data), eye,
                               pipelines[PIPELINE_WINDOWS],
                               pipeline_layout,
                               cmd_buffer, &self->mat_view[eye],
                               &self->mat_projection[eye], TRUE);
    }

  for (GSList *l = xrd_window_manager_get_buttons (manager);
       l != NULL; l = l->next)
    {
      xrd_scene_window_render (XRD_SCENE_WINDOW (l->data), eye,
                               pipelines[PIPELINE_WINDOWS],
                               pipeline_layout,
                               cmd_buffer, &self->mat_view[eye],
                               &self->mat_projection[eye], TRUE);
    }

  _render_pointers (self, eye, cmd_buffer, pipelines, pipeline_layout, &vp);

  _render_selections (self, eye, cmd_buffer, pipelines, pipeline_layout, &vp);

  _render_devices (eye, cmd_buffer, pipeline_layout, pipelines, &vp, self);

  XrdDesktopCursor *cursor = xrd_client_get_desktop_cursor (XRD_CLIENT (self));
  XrdSceneDesktopCursor *scene_cursor = XRD_SCENE_DESKTOP_CURSOR (cursor);
  xrd_scene_window_render (XRD_SCENE_WINDOW (scene_cursor), eye,
                           pipelines[PIPELINE_TIP],
                           pipeline_layout,
                           cmd_buffer, &self->mat_view[eye],
                           &self->mat_projection[eye], FALSE);
}

static gboolean
_init_vulkan (XrdSceneClient   *self,
              GulkanClient     *gc)
{
  GxrContext *context = xrd_client_get_gxr_context (XRD_CLIENT (self));
  if (!xrd_scene_renderer_init_vulkan (self->renderer, context))
    return FALSE;

  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  self->background =
    xrd_scene_background_new (gc, descriptor_set_layout);

  VkBuffer lights =
    xrd_scene_renderer_get_lights_buffer_handle (self->renderer);

  XrdDesktopCursor *cursor =
    XRD_DESKTOP_CURSOR (xrd_scene_desktop_cursor_new (gc, descriptor_set_layout,
                                                      lights));

  xrd_client_set_desktop_cursor (XRD_CLIENT (self), cursor);

  xrd_scene_renderer_set_render_cb (self->renderer, _render_eye_cb, self);
  xrd_scene_renderer_set_update_lights_cb (self->renderer,
                                           _update_lights_cb, self);

  return TRUE;
}

static void
_init_scene_controller (XrdClient     *client,
                        GxrController *controller)
{
  XrdSceneClient *self = XRD_SCENE_CLIENT (client);

  xrd_render_lock ();

  GxrContext *context = xrd_client_get_gxr_context (client);
  GulkanClient *gc = gxr_context_get_gulkan (context);

  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  XrdScenePointer *pointer = xrd_scene_pointer_new (gc, descriptor_set_layout);
  gxr_controller_set_pointer (controller, GXR_POINTER (pointer));

  VkBuffer lights =
    xrd_scene_renderer_get_lights_buffer_handle (self->renderer);

  XrdScenePointerTip *pointer_tip =
    xrd_scene_pointer_tip_new (gc, descriptor_set_layout, lights);
  gxr_controller_set_pointer_tip (controller, GXR_POINTER_TIP (pointer_tip));

  XrdSceneSelection *selection =
    xrd_scene_selection_new (gc, descriptor_set_layout);
  gxr_controller_set_user_data (controller, selection);

  xrd_render_unlock ();
}


void
xrd_scene_client_render (XrdSceneClient *self)
{
  GxrContext *context = xrd_client_get_gxr_context (XRD_CLIENT (self));
  if (!gxr_context_begin_frame (context))
    {
      g_printerr ("Failed to begin frame\n");
    }

  for (uint32_t eye = 0; eye < 2; eye++)
    gxr_context_get_view (context, eye, &self->mat_view[eye]);

  if (!self->have_projection)
    {
      for (uint32_t eye = 0; eye < 2; eye++)
        gxr_context_get_projection (context, eye, self->near, self->far,
                                   &self->mat_projection[eye]);
      self->have_projection = TRUE;
    }

  xrd_render_lock ();
  if (!xrd_scene_renderer_draw (self->renderer))
    {
      g_printerr ("Failed to draw frame\n");
    }

  if (!gxr_context_end_frame (context))
    {
      g_printerr ("Failed to end frame\n");
      xrd_render_unlock ();
      return;
    }
  xrd_render_unlock ();
}

static GulkanClient *
_get_gulkan (XrdClient *client)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  return gxr_context_get_gulkan (context);
}

static XrdWindow *
_window_new_from_meters (XrdClient  *client,
                         const char *title,
                         float       width,
                         float       height,
                         float       ppm)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  GulkanClient *gc = gxr_context_get_gulkan (context);

  XrdSceneClient *self = XRD_SCENE_CLIENT (client);
  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  VkBuffer lights =
    xrd_scene_renderer_get_lights_buffer_handle (self->renderer);

  XrdSceneWindow *window =
    xrd_scene_window_new_from_meters (title, width, height, ppm, gc,
                                      descriptor_set_layout, lights);
  return XRD_WINDOW (window);
}

static XrdWindow *
_window_new_from_data (XrdClient *client, XrdWindowData *data)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  GulkanClient *gc = gxr_context_get_gulkan (context);

  XrdSceneClient *self = XRD_SCENE_CLIENT (client);
  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  VkBuffer lights =
    xrd_scene_renderer_get_lights_buffer_handle (self->renderer);

  XrdSceneWindow *window =
    xrd_scene_window_new_from_data (data, gc, descriptor_set_layout, lights);
  return XRD_WINDOW (window);
}

static XrdWindow *
_window_new_from_pixels (XrdClient  *client,
                         const char *title,
                         uint32_t    width,
                         uint32_t    height,
                         float       ppm)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  GulkanClient *gc = gxr_context_get_gulkan (context);

  XrdSceneClient *self = XRD_SCENE_CLIENT (client);
  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  VkBuffer lights =
    xrd_scene_renderer_get_lights_buffer_handle (self->renderer);

  XrdSceneWindow *window =
    xrd_scene_window_new_from_pixels (title, width, height, ppm, gc,
                                      descriptor_set_layout, lights);
  return XRD_WINDOW (window);
}

static XrdWindow *
_window_new_from_native (XrdClient   *client,
                         const gchar *title,
                         gpointer     native,
                         uint32_t     width_pixels,
                         uint32_t     height_pixels,
                         float        ppm)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  GulkanClient *gc = gxr_context_get_gulkan (context);

  XrdSceneClient *self = XRD_SCENE_CLIENT (client);
  VkDescriptorSetLayout *descriptor_set_layout =
    xrd_scene_renderer_get_descriptor_set_layout (self->renderer);

  VkBuffer lights =
    xrd_scene_renderer_get_lights_buffer_handle (self->renderer);

  XrdSceneWindow *window =
    xrd_scene_window_new_from_native (title, native, width_pixels,
                                      height_pixels, ppm, gc,
                                      descriptor_set_layout, lights);
  return XRD_WINDOW (window);
}

static void
xrd_scene_client_class_init (XrdSceneClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_client_finalize;

  XrdClientClass *xrd_client_class = XRD_CLIENT_CLASS (klass);
  xrd_client_class->get_gulkan = _get_gulkan;
  xrd_client_class->init_controller = _init_scene_controller;

  xrd_client_class->window_new_from_meters = _window_new_from_meters;
  xrd_client_class->window_new_from_data = _window_new_from_data;
  xrd_client_class->window_new_from_pixels = _window_new_from_pixels;
  xrd_client_class->window_new_from_native = _window_new_from_native;
}
