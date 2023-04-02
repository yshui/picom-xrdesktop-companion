/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-client-private.h"

#include <gxr.h>

#include "graphene-ext.h"

#include "xrd-desktop-cursor.h"
#include "xrd-settings.h"
#include "xrd-scene-client-private.h"
#include "xrd-container.h"
#include "xrd-math.h"
#include "xrd-button.h"
#include "xrd-overlay-window.h"
#include "xrd-overlay-client-private.h"
#include "xrd-render-lock.h"
#include "xrd-version.h"

#define WINDOW_MIN_DIST .05f
#define WINDOW_MAX_DIST 15.f

#define APP_NAME "xrdesktop"

enum {
  KEYBOARD_PRESS_EVENT,
  CLICK_EVENT,
  MOVE_CURSOR_EVENT,
  REQUEST_QUIT_EVENT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
  WM_ACTIONSET,
  SYNTH_ACTIONSET,
  LAST_ACTIONSET
};

typedef struct _XrdClientPrivate
{
  GObject parent;

  XrdWindowManager *manager;
  XrdInputSynth *input_synth;

  XrdWindow *button_reset;
  XrdWindow *button_sphere;
  XrdWindow *button_pinned_only;
  XrdWindow *button_selection_mode;
  XrdWindow *button_ignore_input;

  gboolean pinned_only;
  gboolean selection_mode;
  gboolean ignore_input;
  /* ignore_input has two effects:
   * 1. input will only be created for buttons, not windows.
   * 2. only in overlay mode: pointers are only shown pointing at buttons. */

  XrdWindow *keyboard_window;

  gulong keyboard_press_signal;
  gulong keyboard_close_signal;

  guint poll_runtime_event_source_id;
  guint poll_input_source_id;
  guint poll_input_rate_ms;

  double analog_threshold;

  double scroll_to_push_ratio;
  double scroll_to_scale_ratio;

  double pixel_per_meter;

  XrdDesktopCursor *cursor;

  VkImageLayout upload_layout;

  XrdContainer *wm_control_container;

  gint64 last_poll_timestamp;

  /* maps a key to desktop #XrdWindows, but not buttons. */
  GHashTable *window_mapping;

  GxrActionSet *action_sets[LAST_ACTIONSET];

  GxrContext *context;
} XrdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (XrdClient, xrd_client, G_TYPE_OBJECT)

static void
_finalize (GObject *gobject);

static gboolean
_init_buttons (XrdClient *self, GxrController *controller);

static void
_destroy_buttons (XrdClient *self);

static void
_add_window_callbacks (XrdClient *self,
                       XrdWindow *window);

static void
_add_button_callbacks (XrdClient *self,
                       XrdWindow *button);

static void
xrd_client_class_init (XrdClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = _finalize;

  signals[KEYBOARD_PRESS_EVENT] =
    g_signal_new ("keyboard-press-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  signals[CLICK_EVENT] =
    g_signal_new ("click-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  signals[MOVE_CURSOR_EVENT] =
    g_signal_new ("move-cursor-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  signals[REQUEST_QUIT_EVENT] =
    g_signal_new ("request-quit-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static GxrContext *
_create_gxr_context (GxrAppType app_type)
{
  GSList *instance_ext_list =
    gulkan_client_get_external_memory_instance_extensions ();

  GSList *device_ext_list =
    gulkan_client_get_external_memory_device_extensions ();

  GSettings *settings = xrd_settings_get_instance ();
  GxrApi api = (guint) g_settings_get_enum (settings, "default-api");

  GxrContext *context = gxr_context_new_full (app_type,
                                              api,
                                              instance_ext_list,
                                              device_ext_list,
                                              APP_NAME,
                                              XRD_VERSION_HEX);

  g_slist_free_full (instance_ext_list, g_free);
  g_slist_free_full (device_ext_list, g_free);
  return context;
}

static GxrAppType
_xrd_mode_to_gxr_app_type (XrdClientMode mode)
{
  switch (mode)
  {
    case XRD_CLIENT_MODE_OVERLAY:
      return GXR_APP_OVERLAY;
    case XRD_CLIENT_MODE_SCENE:
      return GXR_APP_SCENE;
    default:
      g_printerr ("Unknown client mode: %d\n", mode);
      return GXR_APP_OVERLAY;
  }
}

XrdClient *
xrd_client_new_with_mode (XrdClientMode mode)
{
  GxrAppType app_type = _xrd_mode_to_gxr_app_type (mode);

  GxrContext *context = _create_gxr_context (app_type);
  if (!context)
    {
      g_printerr ("Could not init VR runtime.\n");
      return NULL;
    }

  /* different when GXR_API env var is used or api not available/compiled in */
  GxrApi used_api = gxr_context_get_api (context);

  if (used_api == GXR_API_OPENXR && mode == XRD_CLIENT_MODE_OVERLAY)
    {
      g_warning ("Overlay client is not available on OpenXR. "
                 "Launching scene client.");
      mode = XRD_CLIENT_MODE_SCENE;
    }

  switch (mode)
    {
    case XRD_CLIENT_MODE_OVERLAY:
      return XRD_CLIENT (xrd_overlay_client_new (context));
    case XRD_CLIENT_MODE_SCENE:
      return XRD_CLIENT (xrd_scene_client_new (context));
    }
  g_printerr ("Unknown client mode.");
  return NULL;
}

XrdClient *
xrd_client_new (void)
{
  GSettings *settings = xrd_settings_get_instance ();
  XrdClientMode mode = (guint) g_settings_get_enum (settings, "default-mode");
  return xrd_client_new_with_mode (mode);
}

void
xrd_client_set_upload_layout (XrdClient *self, VkImageLayout layout)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  priv->upload_layout = layout;
}

VkImageLayout
xrd_client_get_upload_layout (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return priv->upload_layout;
}

/**
 * xrd_client_add_container:
 * @self: The #XrdClient
 * @container: The #XrdContainer to add
 *
 * For a container to start behaving according to its layout and attachment,
 * it must be added to the client.
 *
 * Note: windows in the container must be added to the client separately with
 * xrd_client_add_window(), preferably with draggable set to FALSE.
 */
void
xrd_client_add_container (XrdClient *self,
                          XrdContainer *container)
{
  XrdWindowManager *manager = xrd_client_get_manager (self);
  xrd_window_manager_add_container (manager, container);
}

void
xrd_client_remove_container (XrdClient *self,
                             XrdContainer *container)
{
  XrdWindowManager *manager = xrd_client_get_manager (self);
  xrd_window_manager_remove_container (manager, container);
}

/**
 * xrd_client_add_window:
 * @self: The #XrdClient
 * @window: The #XrdWindow to add
 * @draggable: Desktop windows should set this to TRUE. This will enable the
 * expected interaction of being able to grab windows and drag them around.
 * It should be set to FALSE for example for
 *  - child windows
 *  - windows in a container that is attached to the FOV, a controller, etc.
 * @lookup_key: If looking up the #XrdWindow by a key with
 * xrd_client_lookup_window() should be enabled, set to != NULL.
 * Note that an #XrdWindow can be replaced by the overlay-scene switch.
 * Therefore the #XrdWindow should always be looked up instead of cached.
 */
void
xrd_client_add_window (XrdClient *self,
                       XrdWindow *window,
                       gboolean   draggable,
                       gpointer   lookup_key)
{
  XrdWindowFlags flags = XRD_WINDOW_HOVERABLE | XRD_WINDOW_DESTROY_WITH_PARENT;

  /* User can't drag child windows, they are attached to the parent.
   * The child window's position is managed by its parent, not the WM. */
  if (draggable)
    flags |= XRD_WINDOW_DRAGGABLE | XRD_WINDOW_MANAGED;

  XrdWindowManager *manager = xrd_client_get_manager (self);

  xrd_render_lock ();

  xrd_window_manager_add_window (manager, window, flags);

  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  if (priv->pinned_only &&
      !(flags & XRD_WINDOW_BUTTON) &&
      !xrd_window_is_pinned (window))
    {
      xrd_window_hide (window);
    }

  xrd_render_unlock ();

  _add_window_callbacks (self, window);

  if (lookup_key != NULL)
    g_hash_table_insert (priv->window_mapping, lookup_key,
                         xrd_window_get_data (window));
}

XrdWindow *
xrd_client_lookup_window (XrdClient *self,
                          gpointer key)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  XrdWindowData *data = g_hash_table_lookup (priv->window_mapping, key);
  if (!data)
    {
      g_print ("Error looking up window %p\n", key);
      return NULL;
    }
  return data->xrd_window;
}

/**
 * xrd_client_button_new_from_text:
 * @self: The #XrdClient
 * @width: Width in meters.
 * @height: Height in meters.
 * @ppm: Density in pixels per meter
 * @label_count: The number of text lines given in @label
 * @label: One or more lines of text that will be displayed on the button.
 *
 * Creates a button and submits a Cairo rendered text label to it.
 */

XrdWindow*
xrd_client_button_new_from_text (XrdClient *self,
                                 float      width,
                                 float      height,
                                 float      ppm,
                                 int        label_count,
                                 gchar    **label)
{
  GString *full_label = g_string_new ("");
  for (int i = 0; i < label_count; i++)
    {
      g_string_append (full_label, label[i]);
      if (i < label_count - 1)
        g_string_append (full_label, " ");
    }

  XrdWindow *button = xrd_window_new_from_meters (self, full_label->str,
                                                  width, height, ppm);

  g_string_free (full_label, TRUE);

  if (button == NULL)
    {
      g_printerr ("Could not create button.\n");
      return NULL;
    }

  GulkanClient *gc = xrd_client_get_gulkan (self);
  VkImageLayout layout = xrd_client_get_upload_layout (self);

  xrd_button_set_text (button, gc, layout, label_count, label);

  return button;
}

XrdWindow*
xrd_client_button_new_from_icon (XrdClient   *self,
                                 float        width,
                                 float        height,
                                 float        ppm,
                                 const gchar *url)
{
  XrdWindow *button = xrd_window_new_from_meters (self, url,
                                                  width, height, ppm);

  if (button == NULL)
    {
      g_printerr ("Could not create button.\n");
      return NULL;
    }

  GulkanClient *gc = xrd_client_get_gulkan (self);
  VkImageLayout layout = xrd_client_get_upload_layout (self);

  xrd_button_set_icon (button, gc, layout, url);

  return button;
}

/**
 * xrd_client_add_button:
 * @self: The #XrdClient
 * @button: The button (#XrdWindow) to add.
 * @position: World space position of the button.
 * @press_callback: A function that will be called when the button is grabbed.
 * @press_callback_data: User pointer passed to @press_callback.
 *
 * Buttons are special windows that can not be grabbed and dragged around.
 * Instead a button's press_callback is called on the grab action.
 */
void
xrd_client_add_button (XrdClient          *self,
                       XrdWindow          *button,
                       graphene_point3d_t *position,
                       GCallback           press_callback,
                       gpointer            press_callback_data)
{
  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, position);
  xrd_window_set_transformation (button, &transform);

  XrdWindowManager *manager = xrd_client_get_manager (self);

  xrd_window_manager_add_window (manager,
                                 button,
                                 XRD_WINDOW_HOVERABLE |
                                 XRD_WINDOW_DESTROY_WITH_PARENT |
                                 XRD_WINDOW_BUTTON);

  g_signal_connect (button, "grab-start-event",
                    (GCallback) press_callback, press_callback_data);

  _add_button_callbacks (self, button);
}

void
xrd_client_set_pin (XrdClient *self,
                    XrdWindow *win,
                    gboolean pin)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  xrd_window_set_pin (win, pin, priv->pinned_only);
}

void
xrd_client_show_pinned_only (XrdClient *self,
                             gboolean pinned_only)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  priv->pinned_only = pinned_only;

  GSList *windows = xrd_window_manager_get_windows (priv->manager);

  for (GSList *l = windows; l != NULL; l = l->next)
    {
      XrdWindow *window = (XrdWindow *) l->data;
      gboolean pinned = xrd_window_is_pinned (window);
      if (!pinned_only || (pinned_only && pinned))
        xrd_window_show (window);
      else
        xrd_window_hide (window);
    }

  GulkanClient *client = xrd_client_get_gulkan (self);
  VkImageLayout layout = xrd_client_get_upload_layout (self);
  if (priv->button_pinned_only)
    {
      if (pinned_only)
        {
          xrd_button_set_icon (priv->button_pinned_only, client, layout,
                               "/icons/object-hidden-symbolic.svg");
        }
      else
        {
          xrd_button_set_icon (priv->button_pinned_only, client, layout,
                               "/icons/object-visible-symbolic.svg");
        }
    }
}

static void
_device_activate_cb (GxrDeviceManager *device_manager,
                     GxrDevice        *device,
                     gpointer          _self);

/**
 * xrd_client_get_keyboard_window
 * @self: The #XrdClient
 *
 * Returns: The window that is currently used for keyboard input. Can be NULL.
 */
XrdWindow *
xrd_client_get_keyboard_window (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return priv->keyboard_window;
}

GulkanClient *
xrd_client_get_gulkan (XrdClient *self)
{
  XrdClientClass *klass = XRD_CLIENT_GET_CLASS (self);
  if (klass->get_gulkan == NULL)
      return FALSE;
  return klass->get_gulkan (self);
}

static void
_tip_update_texture_res (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointerTip *tip = GXR_POINTER_TIP (_data);

  int width;
  int height;

  GVariant *texture_res = g_settings_get_value (settings, key);
  g_variant_get (texture_res, "(ii)", &width, &height);
  g_variant_unref (texture_res);

  gxr_pointer_tip_update_texture_resolution (tip, width, height);
}

static void
_tip_update_active_color (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointerTip *tip = GXR_POINTER_TIP (_data);

  graphene_point3d_t active_color;
  GVariant *var = g_settings_get_value (settings, key);
  double r, g, b;
  g_variant_get (var, "(ddd)", &r, &g, &b);
  graphene_point3d_init (&active_color, (float) r, (float) g, (float) b);
  g_variant_unref (var);

  gxr_pointer_tip_update_color (tip, TRUE, &active_color);
}

static void
_tip_update_passive_color (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointerTip *tip = GXR_POINTER_TIP (_data);

  graphene_point3d_t passive_color;

  GVariant *var = g_settings_get_value (settings, key);
  double r, g, b;
  g_variant_get (var, "(ddd)", &r, &g, &b);
  graphene_point3d_init (&passive_color, (float) r, (float) g, (float) b);
  g_variant_unref (var);

  gxr_pointer_tip_update_color (tip, FALSE, &passive_color);
}

static void
_tip_update_pulse_alpha (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointerTip *tip = GXR_POINTER_TIP (_data);

  double pulse_alpha = g_settings_get_double (settings, key);

  gxr_pointer_tip_update_pulse_alpha (tip, pulse_alpha);
}

static void
_tip_update_keep_apparent_size (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointerTip *tip = GXR_POINTER_TIP (_data);

  gboolean keep_apparent_size = g_settings_get_boolean (settings, key);

  gxr_pointer_tip_update_keep_apparent_size (tip, keep_apparent_size);
}

static void
_tip_update_width_meters (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointerTip *tip = GXR_POINTER_TIP (_data);

  float width_meters = (float) g_settings_get_double (settings, key);

  gxr_pointer_tip_update_width_meters (tip, width_meters);
}

static void
_pointer_update_render_ray (GSettings *settings, gchar *key, gpointer _data)
{
  GxrPointer *pointer = GXR_POINTER (_data);

  gboolean render_ray = g_settings_get_boolean (settings, key);

  gxr_pointer_update_render_ray (pointer, render_ray);
}

static void
_connect_pointer_tip_settings (GxrPointerTip *tip)
{
  /* tip resolution config has to happen after self->uploader gets set */
  xrd_settings_connect_and_apply (G_CALLBACK (_tip_update_texture_res),
                                  "pointer-tip-resolution", tip);

  xrd_settings_connect_and_apply (G_CALLBACK (_tip_update_passive_color),
                                  "pointer-tip-passive-color", tip);

  xrd_settings_connect_and_apply (G_CALLBACK (_tip_update_active_color),
                                  "pointer-tip-active-color", tip);

  xrd_settings_connect_and_apply (G_CALLBACK (_tip_update_pulse_alpha),
                                  "pointer-tip-pulse-alpha", tip);

  xrd_settings_connect_and_apply (G_CALLBACK (_tip_update_keep_apparent_size),
                                  "pointer-tip-keep-apparent-size", tip);

  xrd_settings_connect_and_apply (G_CALLBACK (_tip_update_width_meters),
                                  "pointer-tip-width-meters", tip);
}

static void
_init_controller (XrdClient     *self,
                  GxrController *controller)
{
  XrdClientClass *klass = XRD_CLIENT_GET_CLASS (self);
  if (klass->init_controller == NULL)
      return;
  klass->init_controller (self, controller);

  GxrPointerTip *tip = gxr_controller_get_pointer_tip (controller);
  _connect_pointer_tip_settings (tip);

  GxrPointer *pointer = gxr_controller_get_pointer (controller);
  xrd_settings_connect_and_apply (G_CALLBACK (_pointer_update_render_ray),
                                  "pointer-ray-enabled", pointer);
}

/**
 * xrd_client_get_synth_hovered:
 * @self: The #XrdClient
 *
 * Returns: If the controller used for synthesizing input is hovering over an
 * #XrdWindow, return this window, else NULL.
 */
XrdWindow *
xrd_client_get_synth_hovered (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (!priv->input_synth)
    {
      g_print ("Error: No window hovered because synth is NULL\n");
      return NULL;
    }

  GxrController *controller =
    xrd_input_synth_get_primary_controller (priv->input_synth);

  /* no controller, nothing hovered */
  if (controller == NULL)
    return NULL;

  XrdWindow *hovered_window =
    XRD_WINDOW (gxr_controller_get_hover_state (controller)->hovered_object);

    return hovered_window;
}

XrdDesktopCursor *
xrd_client_get_desktop_cursor (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return priv->cursor;
}

void
xrd_client_emit_keyboard_press (XrdClient *self,
                                GdkEventKey *event)
{
  g_signal_emit (self, signals[KEYBOARD_PRESS_EVENT], 0, event);
}

void
xrd_client_emit_click (XrdClient *self,
                       XrdClickEvent *event)
{
  g_signal_emit (self, signals[CLICK_EVENT], 0, event);
}

void
xrd_client_emit_move_cursor (XrdClient *self,
                             XrdMoveCursorEvent *event)
{
  g_signal_emit (self, signals[MOVE_CURSOR_EVENT], 0, event);
}

void
xrd_client_emit_system_quit (XrdClient *self,
                             GxrQuitEvent *event)
{
  g_signal_emit (self, signals[REQUEST_QUIT_EVENT], 0, event);
}

GSList *
xrd_client_get_windows (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return xrd_window_manager_get_windows (priv->manager);
}

static void
_finalize (GObject *gobject)
{
  XrdClient *self = XRD_CLIENT (gobject);
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (priv->wm_control_container)
    _destroy_buttons (self);

  if (priv->poll_runtime_event_source_id > 0)
    g_source_remove (priv->poll_runtime_event_source_id);
  if (priv->poll_input_source_id > 0)
    g_source_remove (priv->poll_input_source_id);

  g_hash_table_unref (priv->window_mapping);

  g_object_unref (priv->manager);
  for (int i = 0; i < LAST_ACTIONSET; i++)
    g_clear_object (&priv->action_sets[i]);

  g_clear_object (&priv->cursor);
  g_clear_object (&priv->input_synth);

  g_clear_object (&priv->context);
  g_clear_object (&priv->wm_control_container);

  xrd_settings_destroy_instance ();

  G_OBJECT_CLASS (xrd_client_parent_class)->finalize (gobject);
}

XrdWindowManager *
xrd_client_get_manager (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return priv->manager;
}

static gboolean
_match_value_ptr (gpointer key,
                  gpointer value,
                  gpointer user_data)
{
  (void) key;
  return value == user_data;
}

/**
 * xrd_client_remove_window:
 * @self: The #XrdClient
 * @window: The #XrdWindow to remove.
 *
 * Removes an #XrdWindow from the management of the #XrdClient and the
 * #XrdWindowManager.
 * Note that the #XrdWindow will not be destroyed by this function.
 */
void
xrd_client_remove_window (XrdClient *self,
                          XrdWindow *window)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  g_hash_table_foreach_remove (priv->window_mapping, _match_value_ptr,
                               xrd_window_get_data (window));

  xrd_render_lock ();

  xrd_window_manager_remove_window (priv->manager, window);

  xrd_render_unlock ();
}

XrdInputSynth *
xrd_client_get_input_synth (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return priv->input_synth;
}

gboolean
xrd_client_poll_runtime_events (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  if (!priv->context)
    return FALSE;

  gxr_context_poll_event (priv->context);

  return TRUE;
}

static gboolean
_is_hovering (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      if (gxr_controller_get_hover_state (controller)->hovered_object != NULL)
        return TRUE;
    }
  return FALSE;
}

static gboolean
_is_grabbing (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      if (gxr_controller_get_grab_state (controller)->grabbed_object != NULL)
        return TRUE;
    }
  return FALSE;
}

static gboolean
_is_grabbed (XrdClient *self,
             XrdWindow *window)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      if (gxr_controller_get_grab_state (controller)->grabbed_object == window)
        return TRUE;
    }
  return FALSE;
}

static gboolean
_is_hovered (XrdClient *self,
             XrdWindow *window)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      if (gxr_controller_get_hover_state (controller)->hovered_object == window)
        return TRUE;
    }
  return FALSE;
}

static void
_check_hover_grab (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);

  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);

      /* TODO: handle invalid pointer poses better */
      if (!gxr_controller_is_pointer_pose_valid (controller))
        continue;

      xrd_render_lock ();

      xrd_window_manager_update_controller (priv->manager, controller);

      xrd_render_unlock ();

      GSList *buttons = xrd_window_manager_get_buttons (priv->manager);

      XrdWindow *hovered_window =
        XRD_WINDOW (gxr_controller_get_hover_state (controller)->hovered_object);
      gboolean hovering_window_for_input =
        hovered_window != NULL &&
        g_slist_find (buttons, hovered_window) == NULL;
      XrdWindow *grabbed_window =
        XRD_WINDOW (gxr_controller_get_grab_state (controller)->grabbed_object);

      /* show cursor while synth controller hovers window, but doesn't grab */
      if (controller == xrd_input_synth_get_primary_controller (priv->input_synth)
          && hovering_window_for_input
          && grabbed_window == NULL)
        xrd_desktop_cursor_show (priv->cursor);
    }
}

gboolean
xrd_client_poll_input_events (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  if (!priv->context)
    {
      g_printerr ("Error polling events: No Gxr Context\n");
      priv->poll_input_source_id = 0;
      return FALSE;
    }

  if (priv->action_sets[WM_ACTIONSET] == NULL ||
      priv->action_sets[SYNTH_ACTIONSET] == NULL)
    {
      g_printerr ("Error: Action Set not created!\n");
      return TRUE;
    }

  /* priv->action_sets[0] is wm actionset,
   * priv->action_sets[1] is synth actionset.
   * wm actions are always updated, synth are added when hovering a window */
  gboolean poll_synth = _is_hovering (self) && !_is_grabbing (self);
  uint32_t count = poll_synth ? 2 : 1;

  if (!gxr_action_sets_poll (priv->action_sets, count))
    {
      g_printerr ("Error polling actions\n");
      priv->poll_input_source_id = 0;
      return FALSE;
    }

  xrd_window_manager_poll_window_events (priv->manager, priv->context);

  _check_hover_grab (self);

  priv->last_poll_timestamp = g_get_monotonic_time ();
  return TRUE;
}

static void
_perform_push_pull (XrdClient *self,
                    GxrController *controller,
                    float push_pull_strength,
                    float ms_since_last_poll)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  GxrHoverState *hover_state = gxr_controller_get_hover_state (controller);

  float new_dist =
    hover_state->distance +

    hover_state->distance *
    (float) priv->scroll_to_push_ratio *
    push_pull_strength *

    (ms_since_last_poll / 1000.f);

  if (new_dist < WINDOW_MIN_DIST || new_dist > WINDOW_MAX_DIST)
    return;

  hover_state->distance = new_dist;

  /* TODO: handle this in controller? */
  gxr_pointer_set_length (gxr_controller_get_pointer (controller),
                          hover_state->distance);
}

static void
_action_push_pull_scale_cb (GxrAction      *action,
                            GxrAnalogEvent *event,
                            XrdClient      *self)
{
  (void) action;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrController *controller = event->controller;

  float ms_since_last_poll =
    (float) (g_get_monotonic_time () - priv->last_poll_timestamp) / 1000.f;

  GxrGrabState *grab_state = gxr_controller_get_grab_state (controller);

  if (grab_state->grabbed_object == NULL)
    {
      g_free (event);
      return;
    }

  double x_state = (double) graphene_vec3_get_x (&event->state);
  double y_state = (double) graphene_vec3_get_y (&event->state);

  /* go back to undecided when "stopping" current action,
   * to allow switching actions without letting go of the window. */
  if (fabs (x_state) < priv->analog_threshold &&
      fabs (y_state) < priv->analog_threshold)
    {
      grab_state->transform_lock = GXR_TRANSFORM_LOCK_NONE;
      g_free (event);
      return;
    }

  if (grab_state->transform_lock == GXR_TRANSFORM_LOCK_NONE)
    {
      if (fabs (x_state) > fabs (y_state) &&
          fabs (x_state) > priv->analog_threshold)
        grab_state->transform_lock = GXR_TRANSFORM_LOCK_SCALE;

      else if (fabs (y_state) > fabs (x_state) &&
          fabs (y_state) > priv->analog_threshold)
        grab_state->transform_lock = GXR_TRANSFORM_LOCK_PUSH_PULL;
    }

  if (grab_state->transform_lock == GXR_TRANSFORM_LOCK_SCALE)
    {
      double factor = x_state * priv->scroll_to_scale_ratio;
      xrd_window_manager_scale (priv->manager, grab_state, (float) factor,
                                ms_since_last_poll);
    }
  else if (grab_state->transform_lock == GXR_TRANSFORM_LOCK_PUSH_PULL)
    _perform_push_pull (self, controller, graphene_vec3_get_y (&event->state),
                        ms_since_last_poll);

  g_free (event);
}

static void
_action_push_pull_cb (GxrAction      *action,
                      GxrAnalogEvent *event,
                      XrdClient      *self)
{
  (void) action;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrController *controller = event->controller;

  float ms_since_last_poll =
    (float) (g_get_monotonic_time () - priv->last_poll_timestamp) / 1000.f;

  GxrGrabState *grab_state = gxr_controller_get_grab_state (controller);

  double y_state = (double) graphene_vec3_get_y (&event->state);
  if (grab_state->grabbed_object && fabs (y_state) > priv->analog_threshold)
    _perform_push_pull (self, controller, graphene_vec3_get_y (&event->state),
                        ms_since_last_poll);

  g_free (event);
}

static void
_action_grab_cb (GxrAction       *action,
                 GxrDigitalEvent *event,
                 XrdClient       *self)
{
  (void) action;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrController *controller = event->controller;

  if (event->changed)
    {
      if (event->state == 1)
        xrd_window_manager_check_grab (priv->manager, controller);
      else
        xrd_window_manager_check_release (priv->manager, controller);
    }

  g_free (event);
}

static void
_action_menu_cb (GxrAction       *action,
                 GxrDigitalEvent *event,
                 XrdClient       *self)
{
  (void) action;
  GxrController *controller = event->controller;

  if (event->changed && event->state == 1 &&
      !gxr_controller_get_hover_state (controller)->hovered_object)
    {
      XrdClientPrivate *priv = xrd_client_get_instance_private (self);
      if (priv->wm_control_container)
        {
          g_print ("Destroying buttons!\n");
         _destroy_buttons (self);
        }
      else
        {
          g_print ("Initializing buttons!\n");
         _init_buttons (self, controller);
        }
    }
  g_free (event);
}

typedef struct {
  GxrGrabState *grab_state;
  graphene_quaternion_t from;
  graphene_quaternion_t from_neg;
  graphene_quaternion_t to;
  float interpolate;
  gint64 last_timestamp;
} XrdOrientationTransition;

static gboolean
_interpolate_orientation_cb (gpointer _transition)
{
  XrdOrientationTransition *transition =
    (XrdOrientationTransition*) _transition;

  GxrGrabState *grab_state = transition->grab_state;

  graphene_quaternion_slerp (&transition->from,
                             &transition->to,
                             transition->interpolate,
                             &grab_state->object_rotation);

  graphene_quaternion_slerp (&transition->from_neg,
                             &transition->to,
                             transition->interpolate,
                             &grab_state->inverse_controller_rotation);

  gint64 now = g_get_monotonic_time ();
  float ms_since_last = (float) (now - transition->last_timestamp) / 1000.f;
  transition->last_timestamp = now;

  /* in seconds */
  const float transition_duration = 0.2f;

  transition->interpolate += ms_since_last / 1000.f / transition_duration;

  if (transition->interpolate > 1)
    {
      graphene_quaternion_init_identity (&grab_state->inverse_controller_rotation);
      graphene_quaternion_init_identity (&grab_state->object_rotation);
      g_free (transition);
      return FALSE;
    }

  return TRUE;
}

static void
_action_reset_orientation_cb (GxrAction       *action,
                              GxrDigitalEvent *event,
                              XrdClient       *self)
{
  (void) action;
  (void) self;

  if (!(event->changed && event->state == 1))
    {
      g_free (event);
      return;
    }

  GxrController *controller = event->controller;

  GxrGrabState *grab_state = gxr_controller_get_grab_state (controller);

  XrdOrientationTransition *transition =
    g_malloc (sizeof (XrdOrientationTransition));

  /* TODO: Check if animation is already in progress */

  transition->last_timestamp = g_get_monotonic_time ();
  transition->interpolate = 0.;
  transition->grab_state = grab_state;

  graphene_quaternion_init_identity (&transition->to);
  graphene_quaternion_init_from_quaternion (&transition->from,
                                            &grab_state->object_rotation);
  graphene_quaternion_init_from_quaternion (&transition->from_neg,
                                            &grab_state->inverse_controller_rotation);

  g_timeout_add (10, _interpolate_orientation_cb, transition);

  g_free (event);
}

static void
_mark_windows_for_selection_mode (XrdClient *self);

static void
_window_grab_start_cb (XrdWindow     *window,
                       GxrController *controller,
                       gpointer       _self)
{
  XrdClient *self = XRD_CLIENT (_self);
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (priv->selection_mode)
    {
      gboolean pinned = xrd_window_is_pinned (window);
      /* in selection mode, windows are always visible */
      xrd_window_set_pin (window, !pinned, FALSE);
      _mark_windows_for_selection_mode (self);
      return;
    }

  /* don't grab if this window is already grabbed */
  if (_is_grabbed (self, window))
    return;

  xrd_window_manager_drag_start (priv->manager, controller);

  if (controller== xrd_input_synth_get_primary_controller (priv->input_synth))
    xrd_desktop_cursor_hide (priv->cursor);
}

static void
_window_grab_cb (XrdWindow    *window,
                 XrdGrabEvent *event,
                 gpointer     _self)
{
  (void) window;
  (void) event;
  (void) _self;

  g_free (event);
}

static void
_mark_windows_for_selection_mode (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  XrdWindowManager *manager = xrd_client_get_manager (self);
  if (priv->selection_mode)
    {
      GSList *all = xrd_window_manager_get_windows (manager);
      for (GSList *l = all; l != NULL; l = l->next)
        {
          XrdWindow *win = l->data;

          if (xrd_window_is_pinned (win))
            xrd_window_select (win);
          else
            xrd_window_deselect (win);

          xrd_window_show (win);
        }
    }
  else
    {
      GSList *all = xrd_window_manager_get_windows (manager);
      for (GSList *l = all; l != NULL; l = l->next)
        {
          XrdWindow *win = l->data;

          xrd_window_end_selection (win);
          if (priv->pinned_only)
            {
              if (xrd_window_is_pinned (win))
                xrd_window_show (win);
              else
                xrd_window_hide (win);
            }
        }
    }
}

static void
_button_hover_cb (XrdWindow     *window,
                  XrdHoverEvent *event,
                  gpointer       _self)
{
  (void) _self;
  xrd_window_select (window);

  g_free (event);
}

static void
_window_hover_end_cb (XrdWindow     *window,
                      GxrController *controller,
                      gpointer       _self)
{
  (void) window;
  XrdClient *self = XRD_CLIENT (_self);
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  XrdInputSynth *input_synth = xrd_client_get_input_synth (self);
  xrd_input_synth_reset_press_state (input_synth);

  if (controller == xrd_input_synth_get_primary_controller (input_synth))
    xrd_desktop_cursor_hide (priv->cursor);

  GSList *buttons = xrd_window_manager_get_buttons (priv->manager);

  gpointer next_hovered =
    gxr_controller_get_hover_state (controller)->hovered_object;

  gboolean next_hovered_is_button =
    next_hovered &&
    XRD_IS_WINDOW (next_hovered) &&
    g_slist_find (buttons, next_hovered);

  if (priv->ignore_input && !next_hovered_is_button)
    gxr_controller_hide_pointer (controller);
}

static void
_button_hover_end_cb (XrdWindow     *window,
                      GxrController *controller,
                      gpointer       _self)
{
  (void) controller;
  XrdClient *self = XRD_CLIENT (_self);
  //XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  /* unmark if no controller is hovering over this button */
  if (!_is_hovered (self, window))
    xrd_window_end_selection (window);

  _window_hover_end_cb (window, controller, _self);
}

static void
_button_sphere_press_cb (XrdWindow     *window,
                         GxrController *controller,
                         gpointer       _self)
{
  (void) controller;
  (void) window;
  XrdClient *self = XRD_CLIENT (_self);
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  xrd_window_manager_arrange_sphere (priv->manager, priv->context);
}

static void
_button_reset_press_cb (XrdWindow     *window,
                        GxrController *controller,
                        gpointer       _self)
{
  (void) controller;
  (void) window;
  XrdClient *self = XRD_CLIENT (_self);
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  xrd_window_manager_arrange_reset (priv->manager);
}

static void
_button_pinned_press_cb (XrdWindow     *button,
                         GxrController *controller,
                         gpointer       _self)
{
  (void) controller;
  (void) button;
  XrdClient *self = _self;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (priv->selection_mode)
    return;

  xrd_client_show_pinned_only (self, !priv->pinned_only);
}

static void
_button_select_pinned_press_cb (XrdWindow     *button,
                                GxrController *controller,
                                gpointer       _self)
{
  (void) controller;
  (void) button;
  XrdClient *self = _self;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  priv->selection_mode = !priv->selection_mode;
  _mark_windows_for_selection_mode (self);

  VkImageLayout layout = xrd_client_get_upload_layout (self);

  GulkanClient *client = xrd_client_get_gulkan (self);
  if (priv->selection_mode)
    {
      xrd_button_set_icon (priv->button_selection_mode, client, layout,
                           "/icons/object-select-symbolic.svg");
    }
  else
    {
      xrd_button_set_icon (priv->button_selection_mode, client, layout,
                           "/icons/view-pin-symbolic.svg");
    }

}

static void
_show_pointers (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    gxr_controller_show_pointer (l->data);
}

static void
_hide_pointers (XrdClient *self, gboolean except_button_hover)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GSList *buttons = xrd_window_manager_get_buttons (priv->manager);

  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);
  for (GSList *l = controllers; l; l = l->next)
    {
      XrdWindow *window =
        XRD_WINDOW (gxr_controller_get_hover_state (l->data)->hovered_object);

      if (!(except_button_hover && window && g_slist_find (buttons, window)))
        gxr_controller_hide_pointer (l->data);
    }
}

static void
_button_ignore_input_press_cb (XrdWindow     *button,
                               GxrController *controller,
                               gpointer       _self)
{
  (void) button;
  (void) controller;
  XrdClient *self = _self;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  priv->ignore_input = !priv->ignore_input;

  xrd_window_manager_set_hover_mode (priv->manager,
                                     priv->ignore_input ?
                                         XRD_HOVER_MODE_BUTTONS :
                                         XRD_HOVER_MODE_EVERYTHING);

  VkImageLayout layout = xrd_client_get_upload_layout (self);
  GulkanClient *client = xrd_client_get_gulkan (self);
  if (priv->ignore_input)
    {
      xrd_button_set_icon (priv->button_ignore_input, client, layout,
                           "/icons/input-no-mouse-symbolic.svg");
    }
  else
    {
      xrd_button_set_icon (priv->button_ignore_input, client, layout,
                           "/icons/input-mouse-symbolic.svg");
    }

  if (priv->ignore_input)
    _hide_pointers (self, TRUE);
  else
    _show_pointers (self);
}

static void
_grid_position (float widget_width,
                float widget_height,
                float grid_rows,
                float grid_columns,
                float row,
                float column,
                graphene_matrix_t *relative_transform)
{
  float grid_width = grid_columns * widget_width;
  float grid_height = grid_rows * widget_height;

  float y_offset = grid_height / 2.f -
                   widget_height * row -
                   widget_height / 2.f;

  float x_offset = - grid_width / 2.f +
                   widget_width * column +
                   widget_width / 2.f;

  graphene_point3d_t position;
  graphene_point3d_init (&position, x_offset, y_offset, 0);
  graphene_matrix_init_translate (relative_transform, &position);
}

static gboolean
_init_buttons (XrdClient *self, GxrController *controller)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  int valid_controller_count = 0;
  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *c = GXR_CONTROLLER (l->data);
      if (gxr_controller_is_pointer_pose_valid (c))
        valid_controller_count++;
    }

  /* Controller attached requires more than 1 controller to use.
   * Otherwise use head tracked menu. */
  int attach_controller = valid_controller_count > 1;

  g_debug ("%d controllers active, attaching menu to %s",
           valid_controller_count, attach_controller ? "hand" : "head");

  float w;
  float h;
  float ppm;
  if (attach_controller)
    {
      w = 0.07f;
      h = 0.07f;
      ppm = 1500.0;
    }
  else
    {
      w = 0.25f;
      h = 0.25f;
      ppm = 450.0;
    }

  float rows = 3;
  float columns = 2;

  priv->wm_control_container = xrd_container_new ();
  xrd_container_set_attachment (priv->wm_control_container,
                                attach_controller ?
                                    XRD_CONTAINER_ATTACHMENT_HAND :
                                    XRD_CONTAINER_ATTACHMENT_HEAD,
                                controller);
  xrd_container_set_layout (priv->wm_control_container,
                            XRD_CONTAINER_RELATIVE);

  /* position where button is initially created doesn't matter. */
  graphene_point3d_t position = { .x =  0, .y = 0, .z = 0 };

  graphene_matrix_t relative_transform;

  priv->button_sphere =
    xrd_client_button_new_from_icon (self, w, h, ppm,
                                     "/icons/align-sphere-symbolic.svg");
  if (!priv->button_sphere)
    return FALSE;

  xrd_client_add_button (self, priv->button_sphere, &position,
                         (GCallback) _button_sphere_press_cb, self);

  _grid_position (w, h, rows, columns, 0, 0, &relative_transform);
  xrd_container_add_window (priv->wm_control_container,
                            priv->button_sphere,
                            &relative_transform);

  priv->button_reset =
    xrd_client_button_new_from_icon (self, w, h, ppm,
                                     "/icons/edit-undo-symbolic.svg");
  if (!priv->button_reset)
    return FALSE;

  xrd_client_add_button (self, priv->button_reset, &position,
                         (GCallback) _button_reset_press_cb, self);

  _grid_position (w, h, rows, columns, 0, 1, &relative_transform);
  xrd_container_add_window (priv->wm_control_container,
                            priv->button_reset,
                            &relative_transform);

  priv->button_selection_mode =
    xrd_client_button_new_from_icon (self, w, h, ppm,
                                     "/icons/view-pin-symbolic.svg");
  if (!priv->button_selection_mode)
    return FALSE;

  xrd_client_add_button (self, priv->button_selection_mode, &position,
                         (GCallback) _button_select_pinned_press_cb, self);

  _grid_position (w, h, rows, columns, 1, 0, &relative_transform);
  xrd_container_add_window (priv->wm_control_container,
                            priv->button_selection_mode,
                            &relative_transform);

  if (priv->pinned_only)
    {
      priv->button_pinned_only =
        xrd_client_button_new_from_icon (self, w, h, ppm,
                                         "/icons/object-hidden-symbolic.svg");
    }
  else
    {
      priv->button_pinned_only =
        xrd_client_button_new_from_icon (self, w, h, ppm,
                                         "/icons/object-visible-symbolic.svg");

    }
  if (!priv->button_pinned_only)
    return FALSE;

  xrd_client_add_button (self, priv->button_pinned_only, &position,
                         (GCallback) _button_pinned_press_cb, self);

  _grid_position (w, h, rows, columns, 1, 1, &relative_transform);
  xrd_container_add_window (priv->wm_control_container,
                            priv->button_pinned_only,
                            &relative_transform);

  xrd_client_add_container (self, priv->wm_control_container);


  if (priv->ignore_input)
    {
      priv->button_ignore_input =
        xrd_client_button_new_from_icon (self, w, h, ppm,
                                         "/icons/input-no-mouse-symbolic.svg");
    }
  else
    {
      priv->button_ignore_input =
        xrd_client_button_new_from_icon (self, w, h, ppm,
                                         "/icons/input-mouse-symbolic.svg");
    }
  if (!priv->button_ignore_input)
    return FALSE;

  xrd_client_add_button (self, priv->button_ignore_input, &position,
                         (GCallback) _button_ignore_input_press_cb, self);

  _grid_position (w, h, rows, columns, 2, 0.5f, &relative_transform);
  xrd_container_add_window (priv->wm_control_container,
                            priv->button_ignore_input,
                            &relative_transform);

  if (!attach_controller)
    {
      const float distance = 2.0f;
      xrd_container_center_view (priv->wm_control_container,
                                 priv->context,
                                 distance);
      xrd_container_set_distance (priv->wm_control_container, distance);
    }

  return TRUE;
}

static void
_destroy_buttons (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  xrd_client_remove_window (self, priv->button_sphere);
  g_clear_object (&priv->button_sphere);
  xrd_client_remove_window (self, priv->button_reset);
  g_clear_object (&priv->button_reset);
  xrd_client_remove_window (self, priv->button_pinned_only);
  g_clear_object (&priv->button_pinned_only);
  xrd_client_remove_window (self, priv->button_selection_mode);
  g_clear_object (&priv->button_selection_mode);
  xrd_client_remove_window (self, priv->button_ignore_input);
  g_clear_object (&priv->button_ignore_input);

  xrd_window_manager_remove_container (priv->manager,
                                       priv->wm_control_container);

  g_clear_object (&priv->wm_control_container);
}

static void
_keyboard_press_cb (GxrContext  *context,
                    GdkEventKey *event,
                    XrdClient   *self)
{
  (void) context;
  xrd_client_emit_keyboard_press (self, event);
  gdk_event_free ((GdkEvent*) event);
}

static void
_keyboard_close_cb (GxrContext *context,
                    XrdClient  *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  priv->keyboard_window = NULL;

  g_signal_handler_disconnect(context, priv->keyboard_press_signal);
  g_signal_handler_disconnect(context, priv->keyboard_close_signal);
  priv->keyboard_press_signal = 0;
  priv->keyboard_close_signal = 0;

  if (!priv->ignore_input)
    _show_pointers (self);

  g_print ("Keyboard closed\n");
}

static void
_action_show_keyboard_cb (GxrAction    *action,
                          GxrDigitalEvent *event,
                          XrdClient       *self)
{
  (void) action;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  if (priv->ignore_input)
    {
      g_free (event);
      return;
    }

  if (!event->state && event->changed)
    {

      GxrController *controller =
        xrd_input_synth_get_primary_controller (priv->input_synth);

      /* window hovered by the synthing controller receives input */
      priv->keyboard_window =
        gxr_controller_get_hover_state (controller)->hovered_object;

      if (!priv->keyboard_window)
        {
          g_free (event);
          return;
        }

      gxr_context_show_keyboard (priv->context);

      priv->keyboard_press_signal =
          g_signal_connect (priv->context, "keyboard-press-event",
                            (GCallback) _keyboard_press_cb, self);
      priv->keyboard_close_signal =
          g_signal_connect (priv->context, "keyboard-close-event",
                            (GCallback) _keyboard_close_cb, self);

      _hide_pointers (self, FALSE);
    }

  g_free (event);
}

static void
_window_hover_cb (XrdWindow     *window,
                  XrdHoverEvent *event,
                  XrdClient     *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrController *controller = event->controller;

  gxr_controller_get_hover_state (controller)->hovered_object = window;

  if (event->controller ==
      xrd_input_synth_get_primary_controller (priv->input_synth))
    {
      graphene_matrix_t pointer_pose;
      gxr_controller_get_pointer_pose (controller, &pointer_pose);

      xrd_input_synth_move_cursor (priv->input_synth, window,
                                   &pointer_pose, &event->point);

      xrd_desktop_cursor_update (priv->cursor, priv->context,
                                 window, &event->point);

      if (gxr_controller_get_hover_state (controller)->hovered_object != window)
        xrd_input_synth_reset_scroll (priv->input_synth);
    }

  g_free (event);
}

static void
_window_hover_start_cb (XrdWindow     *window,
                        GxrController *controller,
                        XrdClient     *self)
{
  (void) window;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (priv->ignore_input)
    gxr_controller_show_pointer (controller);
}

static void
_manager_no_hover_cb (XrdWindowManager *manager,
                      XrdNoHoverEvent  *event,
                      XrdClient        *self)
{
  (void) manager;

  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (xrd_input_synth_get_primary_controller (priv->input_synth) ==
      event->controller)
    xrd_input_synth_reset_scroll (priv->input_synth);

  g_free (event);
}

static void
_update_input_poll_rate (GSettings *settings, gchar *key, gpointer _self)
{
  XrdClient *self = _self;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (priv->poll_input_source_id != 0)
    g_source_remove (priv->poll_input_source_id);
  priv->poll_input_rate_ms = g_settings_get_uint (settings, key);

  priv->poll_input_source_id =
      g_timeout_add (priv->poll_input_rate_ms,
      (GSourceFunc) xrd_client_poll_input_events,
      self);
}

static void
_synth_click_cb (XrdInputSynth *synth,
                 XrdClickEvent *event,
                 XrdClient     *self)
{
  (void) synth;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrController *controller = event->controller;

  XrdWindow *hover_window =
    gxr_controller_get_hover_state (controller)->hovered_object;

  if (!hover_window)
    {
      g_free (event);
      return;
    }

  event->window = XRD_WINDOW (hover_window);

  GSList *buttons = xrd_window_manager_get_buttons (priv->manager);
  gboolean is_button = g_slist_find (buttons, hover_window) != NULL;

  if (!is_button)
    {
      /* in window selection mode don't handle clicks on non-button windows */
      if (priv->selection_mode)
        {
          g_free (event);
          return;
        }
      else
        xrd_client_emit_click (self, event);
    }


  if (event->button == LEFT_BUTTON && event->state)
      {
        gxr_pointer_tip_animate_pulse (
          gxr_controller_get_pointer_tip (controller));

        if (is_button)
          xrd_window_emit_grab_start (event->window, event->controller);
      }

  g_free (event);
}

static void
_synth_move_cursor_cb (XrdInputSynth      *synth,
                       XrdMoveCursorEvent *event,
                       XrdClient          *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  if (priv->selection_mode)
    return;

  (void) synth;
  if (!event->ignore)
    xrd_client_emit_move_cursor (self, event);

  g_free (event);
}

XrdWindow *
xrd_client_window_new_from_meters (XrdClient  *self,
                                   const char *title,
                                   float       width,
                                   float       height,
                                   float       ppm)
{
  XrdClientClass *klass = XRD_CLIENT_GET_CLASS (self);
  if (klass->window_new_from_meters == NULL)
    return NULL;

  xrd_render_lock ();
  XrdWindow *window =
    klass->window_new_from_meters (self, title, width, height, ppm);
  xrd_render_unlock ();
  return window;
}

XrdWindow *
xrd_client_window_new_from_data (XrdClient  *self,
                                 XrdWindowData *data)
{
  XrdClientClass *klass = XRD_CLIENT_GET_CLASS (self);
  if (klass->window_new_from_data == NULL)
    return NULL;

  xrd_render_lock ();
  XrdWindow *window =
    klass->window_new_from_data (self, data);
  if (window)
    data->owned_by_window = TRUE;
  xrd_render_unlock ();
  return window;
}

XrdWindow *
xrd_client_window_new_from_pixels (XrdClient  *self,
                                   const char *title,
                                   uint32_t    width,
                                   uint32_t    height,
                                   float       ppm)
{
  XrdClientClass *klass = XRD_CLIENT_GET_CLASS (self);
  if (klass->window_new_from_pixels == NULL)
    return NULL;
  xrd_render_lock ();
  XrdWindow *window =
    klass->window_new_from_pixels (self, title, width, height, ppm);
  xrd_render_unlock ();
  return window;
}

XrdWindow *
xrd_client_window_new_from_native (XrdClient   *self,
                                   const gchar *title,
                                   gpointer     native,
                                   uint32_t     width_pixels,
                                   uint32_t     height_pixels,
                                   float        ppm)
{
  XrdClientClass *klass = XRD_CLIENT_GET_CLASS (self);
  if (klass->window_new_from_native == NULL)
    return NULL;
  xrd_render_lock ();
  XrdWindow *window =
    klass->window_new_from_native (self, title, native,
                                   width_pixels, height_pixels, ppm);
  xrd_render_unlock ();
  return window;
}

static void
_add_button_callbacks (XrdClient *self,
                       XrdWindow *button)
{
  g_signal_connect (button, "hover-event",
                    (GCallback) _button_hover_cb, self);

  g_signal_connect (button, "hover-start-event",
                    (GCallback) _window_hover_start_cb, self);

  g_signal_connect (button, "hover-end-event",
                    (GCallback) _button_hover_end_cb, self);
}

static void
_add_window_callbacks (XrdClient *self,
                       XrdWindow *window)
{
  g_signal_connect (window, "grab-start-event",
                    (GCallback) _window_grab_start_cb, self);
  g_signal_connect (window, "grab-event",
                    (GCallback) _window_grab_cb, self);
  // g_signal_connect (window, "release-event",
  //                   (GCallback) _window_release_cb, self);
  g_signal_connect (window, "hover-start-event",
                    (GCallback) _window_hover_start_cb, self);
  g_signal_connect (window, "hover-event",
                    (GCallback) _window_hover_cb, self);
  g_signal_connect (window, "hover-end-event",
                    (GCallback) _window_hover_end_cb, self);
}

void
xrd_client_set_desktop_cursor (XrdClient        *self,
                               XrdDesktopCursor *cursor)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  priv->cursor = cursor;
  xrd_desktop_cursor_hide (priv->cursor);
}

GSList *
xrd_client_get_controllers (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  return gxr_device_manager_get_controllers (dm);
}

static void
_device_activate_cb (GxrDeviceManager *device_manager,
                     GxrDevice        *device,
                     gpointer          _self)
{
  (void) device_manager;

  if (!gxr_device_is_controller (device))
    return;

  GxrController *controller = GXR_CONTROLLER (device);

  XrdClient *self = _self;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  g_debug ("client: Controller %lu activated.\n",
           gxr_device_get_handle (GXR_DEVICE (controller)));

  _init_controller (self, controller);

  if (xrd_input_synth_get_primary_controller(priv->input_synth) == NULL)
    xrd_input_synth_make_primary (priv->input_synth, controller);
}

static void
_device_deactivate_cb (GxrDeviceManager *device_manager,
                       GxrDevice        *device,
                       gpointer          _self)
{
  (void) device_manager;

  if (!gxr_device_is_controller (device))
    return;

  GxrController *controller = GXR_CONTROLLER (device);

  XrdClient *self = _self;
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  g_debug ("client: Controller %lu deactivated..\n",
           gxr_device_get_handle (GXR_DEVICE (controller)));

  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  if (xrd_input_synth_get_primary_controller (priv->input_synth) == controller &&
      g_slist_length (controllers) > 0)
    xrd_input_synth_make_primary ( priv->input_synth, controller);
}

static void
xrd_client_init (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  priv->window_mapping = g_hash_table_new (g_direct_hash, g_direct_equal);

  xrd_settings_connect_and_apply (G_CALLBACK (xrd_settings_update_double_val),
                                  "scroll-to-push-ratio",
                                  &priv->scroll_to_push_ratio);
  xrd_settings_connect_and_apply (G_CALLBACK (xrd_settings_update_double_val),
                                  "scroll-to-scale-ratio",
                                  &priv->scroll_to_scale_ratio);
  xrd_settings_connect_and_apply (G_CALLBACK (xrd_settings_update_double_val),
                                  "analog-threshold", 
                                  &priv->analog_threshold);
  xrd_settings_connect_and_apply (G_CALLBACK (xrd_settings_update_gboolean_val),
                                  "show-only-pinned-startup", 
                                  &priv->pinned_only);

  priv->poll_runtime_event_source_id = 0;
  priv->poll_input_source_id = 0;
  priv->keyboard_window = NULL;
  priv->keyboard_press_signal = 0;
  priv->keyboard_close_signal = 0;
  priv->selection_mode = FALSE;
  priv->ignore_input = FALSE;
  for (int i = 0; i < LAST_ACTIONSET; i++)
    priv->action_sets[i] = NULL;
  priv->cursor = NULL;
  priv->wm_control_container = NULL;

  priv->manager = xrd_window_manager_new ();

  priv->last_poll_timestamp = g_get_monotonic_time ();

  priv->context = NULL;
}

void
xrd_client_set_context (XrdClient *self, GxrContext *context)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  priv->context = context;

  GxrDeviceManager *dm = gxr_context_get_device_manager (context);

  g_signal_connect (dm, "device-activate-event",
                    (GCallback) _device_activate_cb, self);
  g_signal_connect (dm, "device-deactivate-event",
                    (GCallback) _device_deactivate_cb, self);
}

static void
_system_quit_cb (GxrContext   *context,
                 GxrQuitEvent *event,
                 XrdClient    *self)
{
  (void) event;
  (void) self;
  /* g_print("Handling VR quit event %d\n", event->reason); */
  gxr_context_acknowledge_quit (context);

  xrd_client_emit_system_quit (self, event);
  g_free (event);
}

static void
_update_grab_window_threshold (GSettings *settings,
                               gchar *key,
                               gpointer _action)
{
  GxrAction *action = GXR_ACTION (_action);

  float threshold = (float) g_settings_get_double (settings, key);

  gxr_action_set_digital_from_float_threshold (action, threshold);
}

static GxrActionSet *
_create_wm_action_set (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  GxrActionSet *set = gxr_action_set_new_from_url (priv->context,
                                                   "/actions/wm");

  GxrDeviceManager *dm = gxr_context_get_device_manager (priv->context);
  gxr_device_manager_connect_pose_actions (dm, priv->context, set,
                                           "/actions/wm/in/hand_pose",
                                           "/actions/wm/in/hand_pose_hand_grip");

  GxrAction *grab_window =
    gxr_action_set_connect_digital_from_float (set, priv->context,
                                               "/actions/wm/in/grab_window",
                                               0.25f,
                                               "/actions/wm/out/haptic",
                                               (GCallback) _action_grab_cb,
                                               self);
  gxr_action_set_connect (set, priv->context, GXR_ACTION_DIGITAL,
                          "/actions/wm/in/reset_orientation",
                          (GCallback) _action_reset_orientation_cb, self);
  gxr_action_set_connect (set, priv->context, GXR_ACTION_DIGITAL,
                          "/actions/wm/in/menu",
                          (GCallback) _action_menu_cb, self);
  gxr_action_set_connect (set, priv->context, GXR_ACTION_VEC2F,
                          "/actions/wm/in/push_pull_scale",
                          (GCallback) _action_push_pull_scale_cb, self);
  gxr_action_set_connect (set, priv->context, GXR_ACTION_VEC2F,
                          "/actions/wm/in/push_pull",
                          (GCallback) _action_push_pull_cb, self);

  gxr_action_set_connect (set, priv->context, GXR_ACTION_DIGITAL,
                          "/actions/wm/in/show_keyboard",
                          (GCallback) _action_show_keyboard_cb, self);

  xrd_settings_connect_and_apply (G_CALLBACK (_update_grab_window_threshold),
                                  "grab-window-threshold", grab_window);

  return set;
}

static void
_init_gxr_callbacks (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  g_signal_connect (priv->context, "quit-event",
                    (GCallback) _system_quit_cb, self);

  priv->poll_runtime_event_source_id =
    g_timeout_add (20, (GSourceFunc) xrd_client_poll_runtime_events, self);
}

void
xrd_client_init_input_callbacks (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);

  if (gxr_context_get_api (priv->context) == GXR_API_OPENVR)
    {
      if (!gxr_context_load_action_manifest (
            priv->context,
            "xrdesktop.openvr",
            "/res/bindings/openvr",
            "actions.json"))
        {
          g_print ("Failed to load action bindings!\n");
          return;
        }
    }
  else
    {
      {
        if (!gxr_context_load_action_manifest (
          priv->context,
          "xrdesktop.openxr",
          "/res/bindings/openxr",
          "actions.json"))
        {
          g_print ("Failed to load action bindings!\n");
          return;
        }
      }
    }

  priv->input_synth = xrd_input_synth_new ();
  priv->button_sphere = NULL;
  priv->button_reset = NULL;
  priv->button_pinned_only = NULL;
  priv->button_selection_mode = NULL;
  priv->button_ignore_input = NULL;
  priv->wm_control_container = NULL;

  priv->action_sets[WM_ACTIONSET] = _create_wm_action_set (self);
  priv->action_sets[SYNTH_ACTIONSET] =
    xrd_input_synth_create_action_set (priv->input_synth, priv->context);

  gxr_action_sets_attach_bindings (priv->action_sets, priv->context, 2);

  _init_gxr_callbacks (self);

  xrd_settings_connect_and_apply (G_CALLBACK (_update_input_poll_rate),
                                  "input-poll-rate-ms", self);

  g_signal_connect (priv->manager, "no-hover-event",
                    (GCallback) _manager_no_hover_cb, self);

  g_signal_connect (priv->input_synth, "click-event",
                    (GCallback) _synth_click_cb, self);
  g_signal_connect (priv->input_synth, "move-cursor-event",
                    (GCallback) _synth_move_cursor_cb, self);
}

static XrdClient *
_replace_client (XrdClient *self)
{
  gboolean to_scene = XRD_IS_OVERLAY_CLIENT (self);
  g_object_unref (self);

  GxrAppType new_app_type = to_scene ? GXR_APP_SCENE : GXR_APP_OVERLAY;

  GxrContext *context = _create_gxr_context (new_app_type);
  if (!context)
    {
      g_printerr ("Could not init VR runtime.\n");
      return NULL;
    }

  if (to_scene)
    return XRD_CLIENT (xrd_scene_client_new (context));
  else
    return XRD_CLIENT (xrd_overlay_client_new (context));
}

static void
_insert_into_new_hash_table (gpointer key,
                             gpointer value,
                             gpointer new_hash_table)
{
  g_hash_table_insert (new_hash_table, key, value);
}

static GHashTable *
_g_hash_table_clone_direct (GHashTable *old_table)
{
  GHashTable *new_table = g_hash_table_new (g_direct_hash, g_direct_equal);
  g_hash_table_foreach (old_table, _insert_into_new_hash_table, new_table);
  return new_table;
}

static GSList *
_get_new_window_data_list (XrdClient *self)
{
  GSList *data = NULL;
  GSList *windows = xrd_client_get_windows (self);
  for (GSList *l = windows; l; l = l->next)
    {
      XrdWindowData *d = xrd_window_get_data (l->data);
      d->owned_by_window = FALSE;
      data = g_slist_append (data, d);
    }
  return data;
}

/**
 * xrd_client_switch_mode:
 * @self: current #XrdClient to be destroyed.
 *
 * References to gulkan, gxr and xrdesktop objects (like #XrdWindow)
 * will be invalid after calling this function.
 *
 * xrd_client_switch_mode() replaces each #XrdWindow with an appropriate new
 * one, preserving its transformation matrix, scaling, pinned status, etc.
 *
 * The caller is responsible for reconnecting callbacks to #XrdClient signals.
 * The caller is responsible to not use references to any previous #XrdWindow.
 * Pointers to #XrdWindowData will remain valid, however
 * #XrdWindowData->xrd_window will point to a new #XrdWindow.
 *
 * Returns: A new #XrdClient of the opposite mode than the passed one.
 */
struct _XrdClient *
xrd_client_switch_mode (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  gboolean show_only_pinned = priv->pinned_only;
  gboolean ignore_input = priv->ignore_input;

  /* original hash table will be destroyed on XrdClient destroy */
  GHashTable *window_mapping =
    _g_hash_table_clone_direct (priv->window_mapping);

  XrdWindowManager *manager = xrd_client_get_manager (self);

  /* this list preserves the order in which windows were added */
  GSList *window_data_list = _get_new_window_data_list (self);
  for (GSList *l = window_data_list; l; l = l->next)
    {
      XrdWindowData *window_data = l->data;
      if (window_data->texture)
         g_clear_object (&window_data->texture);
      xrd_client_remove_window (self, window_data->xrd_window);
      g_clear_object (&window_data->xrd_window);
    }

  XrdClient *ret = _replace_client (self);
  manager = xrd_client_get_manager (ret);
  priv = xrd_client_get_instance_private (ret);
  priv->window_mapping = window_mapping;

  for (GSList *l = window_data_list; l; l = l->next)
    {
      XrdWindowData *window_data = l->data;
      XrdWindow *window = xrd_client_window_new_from_data (ret, window_data);
      window_data->xrd_window = window;
      gboolean draggable = window_data->parent_window == NULL;

      xrd_client_add_window (ret, window, draggable, NULL);
    }
  g_slist_free (window_data_list);

  xrd_client_show_pinned_only (ret, show_only_pinned);

  priv->ignore_input = ignore_input;

  xrd_window_manager_set_hover_mode (manager, ignore_input ?
                                         XRD_HOVER_MODE_BUTTONS :
                                         XRD_HOVER_MODE_EVERYTHING);

  XrdClientPrivate *ret_priv = xrd_client_get_instance_private (ret);
  if (ret_priv->ignore_input)
    _hide_pointers (ret, TRUE);
  else
    _show_pointers (ret);

  return ret;
}

GxrContext *
xrd_client_get_gxr_context (XrdClient *self)
{
  XrdClientPrivate *priv = xrd_client_get_instance_private (self);
  return priv->context;
}
