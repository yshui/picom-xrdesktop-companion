/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <glib/gprintf.h>
#include <gdk/gdk.h>

#include "gxr-time.h"
#include "gxr-config.h"
#include "gxr-overlay-private.h"

#include "openvr-overlay.h"
#include "openvr-context.h"
#include "openvr-math.h"
#include "openvr-compositor.h"
#include "openvr-functions.h"

#define OVERLAY_CHECK_ERROR(fun, res) \
{ \
  if (res != EVROverlayError_VROverlayError_None) \
    { \
      g_printerr ("ERROR: " fun ": failed with %s in %s:%d\n", \
                  f->overlay->GetOverlayErrorNameFromEnum (res), __FILE__, __LINE__); \
      return FALSE; \
    } \
}

struct _OpenVROverlay
{
  GxrOverlayClass parent;
  VROverlayHandle_t overlay_handle;
  VROverlayHandle_t thumbnail_handle;
};

static struct VRTextureBounds_t defaultBounds = { 0., 0., 1., 1. };
static struct VRTextureBounds_t flippedBounds = { 0., 1., 1., 0. };

G_DEFINE_TYPE (OpenVROverlay, openvr_overlay, GXR_TYPE_OVERLAY)

static void
openvr_overlay_init (OpenVROverlay *self)
{
  self->overlay_handle = 0;
  self->thumbnail_handle = 0;
}

static gboolean
_create (OpenVROverlay *overlay, gchar* key)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  /* k_unVROverlayMaxKeyLength is the limit including the null terminator */
  if (strlen(key) + 1 > k_unVROverlayMaxKeyLength)
    {
      g_printerr ("Overlay key too long, must be shorter than %ld characters\n",
                  (long)k_unMaxSettingsKeyLength - 1);
      return FALSE;
    }

  gchar *key_trimmed = g_strndup (key, k_unVROverlayMaxNameLength - 1);
  err = f->overlay->CreateOverlay (key_trimmed,
                                   key_trimmed,
                                   &self->overlay_handle);

  free (key_trimmed);

  OVERLAY_CHECK_ERROR ("CreateOverlay", err)

  return TRUE;
}

OpenVROverlay *
openvr_overlay_new (gchar* key)
{
  OpenVROverlay *self = (OpenVROverlay*) g_object_new (OPENVR_TYPE_OVERLAY, 0);
  if (!_create(self, key))
    {
      g_object_unref (self);
      return NULL;
    }

  if (self->overlay_handle == k_ulOverlayHandleInvalid)
    {
      g_object_unref (self);
      return NULL;
    }

  return self;
}

static gboolean
_destroy (OpenVROverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->DestroyOverlay (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("DestroyOverlay", err)

  return TRUE;
}

static void
_finalize (GObject *gobject)
{
  OpenVROverlay *self = OPENVR_OVERLAY (gobject);
  _destroy (self);
  G_OBJECT_CLASS (openvr_overlay_parent_class)->finalize (gobject);
}

static gboolean
_show (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->ShowOverlay (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("ShowOverlay", err)
  return TRUE;
}

static gboolean
_hide (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->HideOverlay (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("HideOverlay", err)
  return TRUE;
}

static gboolean
_is_visible (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  return f->overlay->IsOverlayVisible (self->overlay_handle);
}

static gboolean
_thumbnail_is_visible (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  return f->overlay->IsOverlayVisible (self->thumbnail_handle);
}

static gboolean
_set_sort_order (GxrOverlay *overlay, uint32_t sort_order)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->SetOverlaySortOrder (self->overlay_handle, sort_order);

  OVERLAY_CHECK_ERROR ("SetOverlaySortOrder", err)
  return TRUE;
}

static guint
_vr_to_gdk_mouse_button (uint32_t btn)
{
  switch(btn)
  {
    case EVRMouseButton_VRMouseButton_Left:
      return 0;
    case EVRMouseButton_VRMouseButton_Right:
      return 1;
    case EVRMouseButton_VRMouseButton_Middle:
      return 2;
    default:
      return 0;
  }
}

static gboolean
_enable_mouse_input (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->SetOverlayInputMethod (self->overlay_handle,
                                           VROverlayInputMethod_Mouse);

  OVERLAY_CHECK_ERROR ("SetOverlayInputMethod", err)

  return TRUE;
}

static void
_poll_event (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  struct VREvent_t vr_event;

  OpenVRFunctions *f = openvr_get_functions ();

  while (f->overlay->PollNextOverlayEvent (self->overlay_handle, &vr_event,
                                           sizeof (vr_event)))
  {
    switch (vr_event.eventType)
    {
      case EVREventType_VREvent_MouseMove:
      {
        GdkEvent *event = gdk_event_new (GDK_MOTION_NOTIFY);
        event->motion.x = vr_event.data.mouse.x;
        event->motion.y = vr_event.data.mouse.y;
        event->motion.time =
          gxr_time_age_secs_to_monotonic_msecs (vr_event.eventAgeSeconds);
        gxr_overlay_emit_motion_notify (overlay, event);
      } break;

      case EVREventType_VREvent_MouseButtonDown:
      {
        GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);
        event->button.x = vr_event.data.mouse.x;
        event->button.y = vr_event.data.mouse.y;
        event->button.time =
          gxr_time_age_secs_to_monotonic_msecs (vr_event.eventAgeSeconds);
        event->button.button =
          _vr_to_gdk_mouse_button (vr_event.data.mouse.button);
        gxr_overlay_emit_button_press (overlay, event);
      } break;

      case EVREventType_VREvent_MouseButtonUp:
      {
        GdkEvent *event = gdk_event_new (GDK_BUTTON_RELEASE);
        event->button.x = vr_event.data.mouse.x;
        event->button.y = vr_event.data.mouse.y;
        event->button.time =
          gxr_time_age_secs_to_monotonic_msecs (vr_event.eventAgeSeconds);
        event->button.button =
          _vr_to_gdk_mouse_button (vr_event.data.mouse.button);
        gxr_overlay_emit_button_release (overlay, event);
      } break;

      case EVREventType_VREvent_OverlayShown:
        gxr_overlay_emit_show (overlay);
        break;
      case EVREventType_VREvent_OverlayHidden:
        gxr_overlay_emit_hide (overlay);
        break;
      case EVREventType_VREvent_Quit:
        gxr_overlay_emit_destroy (overlay);
        break;

      // TODO: code duplication with system keyboard in OpenVRContext
      case EVREventType_VREvent_KeyboardCharInput:
      {
        // TODO: https://github.com/ValveSoftware/openvr/issues/289
        char *new_input = (char*) vr_event.data.keyboard.cNewInput;

        int len = 0;
        for (; len < 8 && new_input[len] != 0; len++);

        GdkEvent *event = gdk_event_new (GDK_KEY_PRESS);
        event->key.state = TRUE;
        event->key.string = new_input;
        event->key.length = len;
        gxr_overlay_emit_keyboard_press (overlay, event);
      } break;

      case EVREventType_VREvent_KeyboardClosed:
      {
        gxr_overlay_emit_keyboard_close (overlay);
      } break;
    }
  }
}

static gboolean
_set_mouse_scale (GxrOverlay *overlay, float width, float height)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  struct HmdVector2_t mouse_scale = {{ width, height }};
  err = f->overlay->SetOverlayMouseScale (self->overlay_handle, &mouse_scale);

  OVERLAY_CHECK_ERROR ("SetOverlayMouseScale", err)
  return TRUE;
}

static gboolean
_clear_texture (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->ClearOverlayTexture (self->overlay_handle);

  OVERLAY_CHECK_ERROR ("ClearOverlayTexture", err)
  return TRUE;
}

static gboolean
_get_color (GxrOverlay *overlay, graphene_vec3_t *color)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  float r, g, b;
  err = f->overlay->GetOverlayColor (self->overlay_handle, &r, &g, &b);

  OVERLAY_CHECK_ERROR ("GetOverlayColor", err)

  graphene_vec3_init (color, r, g, b);

  return TRUE;
}

static gboolean
_set_color (GxrOverlay *overlay, const graphene_vec3_t *color)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->SetOverlayColor (self->overlay_handle,
                                     graphene_vec3_get_x (color),
                                     graphene_vec3_get_y (color),
                                     graphene_vec3_get_z (color));

  OVERLAY_CHECK_ERROR ("SetOverlayColor", err)
  return TRUE;
}

static gboolean
_set_alpha (GxrOverlay *overlay, float alpha)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->SetOverlayAlpha (self->overlay_handle, alpha);

  OVERLAY_CHECK_ERROR ("SetOverlayAlpha", err)
  return TRUE;
}

static gboolean
_set_width_meters (GxrOverlay *overlay, float meters)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->SetOverlayWidthInMeters (self->overlay_handle, meters);

  OVERLAY_CHECK_ERROR ("SetOverlayWidthInMeters", err)
  return TRUE;
}

static gboolean
_set_transform_absolute (GxrOverlay *overlay,
                         graphene_matrix_t *mat)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  HmdMatrix34_t translation34;
  openvr_math_graphene_to_matrix34 (mat, &translation34);

  enum ETrackingUniverseOrigin origin = f->compositor->GetTrackingSpace ();

  err = f->overlay->SetOverlayTransformAbsolute (self->overlay_handle,
                                                 origin, &translation34);

  OVERLAY_CHECK_ERROR ("SetOverlayTransformAbsolute", err)
  return TRUE;
}

static gboolean
_get_transform_absolute (GxrOverlay *overlay,
                         graphene_matrix_t *mat)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  HmdMatrix34_t translation34;

  enum ETrackingUniverseOrigin origin = f->compositor->GetTrackingSpace ();

  err = f->overlay->GetOverlayTransformAbsolute (self->overlay_handle,
                                                &origin,
                                                &translation34);

  openvr_math_matrix34_to_graphene (&translation34, mat);

  OVERLAY_CHECK_ERROR ("GetOverlayTransformAbsolute", err)
  return TRUE;
}

static gboolean
_set_raw (GxrOverlay *overlay, guchar *pixels,
          uint32_t width, uint32_t height, uint32_t depth)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->SetOverlayRaw (self->overlay_handle,
                                   (void*) pixels, width, height, depth);

  OVERLAY_CHECK_ERROR ("SetOverlayRaw", err)
  return TRUE;
}

static gboolean
_get_size_pixels (GxrOverlay *overlay, VkExtent2D *size)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err =  f->overlay->GetOverlayTextureSize (self->overlay_handle,
                                           &size->width, &size->height);

  OVERLAY_CHECK_ERROR ("GetOverlayTextureSize", err)

  return TRUE;
}

static gboolean
_get_width_meters (GxrOverlay *overlay, float *width)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  err = f->overlay->GetOverlayWidthInMeters (self->overlay_handle, width);

  OVERLAY_CHECK_ERROR ("GetOverlayWidthInMeters", err)

  return TRUE;
}

static gboolean
_show_keyboard (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;
#if (OPENVR_VERSION_MINOR >= 10)
  err = f->overlay->ShowKeyboardForOverlay (
    self->overlay_handle,
    EGamepadTextInputMode_k_EGamepadTextInputModeNormal,
    EGamepadTextInputLineMode_k_EGamepadTextInputLineModeSingleLine,
    EKeyboardFlags_KeyboardFlag_Minimal,
    "OpenVR Overlay Keyboard", 1, "", 0);
#else
  err = f->overlay->ShowKeyboardForOverlay (
    self->overlay_handle,
    EGamepadTextInputMode_k_EGamepadTextInputModeNormal,
    EGamepadTextInputLineMode_k_EGamepadTextInputLineModeSingleLine,
    "OpenVR Overlay Keyboard", 1, "", TRUE, 0);
#endif
  OVERLAY_CHECK_ERROR ("ShowKeyboardForOverlay", err)

  return TRUE;
}

static void
_set_keyboard_position (GxrOverlay      *overlay,
                        graphene_vec2_t *top_left,
                        graphene_vec2_t *bottom_right)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();

  HmdRect2_t rect = {
    .vTopLeft = {
      .v[0] = graphene_vec2_get_x (top_left),
      .v[1] = graphene_vec2_get_y (top_left)
    },
    .vBottomRight = {
      .v[0] = graphene_vec2_get_x (bottom_right),
      .v[1] = graphene_vec2_get_y (bottom_right)
    }
  };
  f->overlay->SetKeyboardPositionForOverlay (self->overlay_handle, rect);
}

static void
_set_flip_y (GxrOverlay *overlay,
             gboolean flip_y)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();

  if (flip_y != gxr_overlay_get_flip_y (overlay))
    {
      VRTextureBounds_t *bounds = flip_y ? &flippedBounds : &defaultBounds;
      f->overlay->SetOverlayTextureBounds (self->overlay_handle, bounds);
    }
}

/* Submit frame to OpenVR runtime */
static gboolean
_submit_texture (GxrOverlay    *overlay,
                 GulkanClient  *client,
                 GulkanTexture *texture)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);
  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  GulkanDevice *device = gulkan_client_get_device (client);

  VkExtent2D extent = gulkan_texture_get_extent (texture);

  GulkanQueue *queue = gulkan_device_get_graphics_queue (device);

  struct VRVulkanTextureData_t texture_data =
    {
      .m_nImage = (uint64_t)
        (struct VkImage_T *)gulkan_texture_get_image (texture),
      .m_pDevice = gulkan_device_get_handle (device),
      .m_pPhysicalDevice = gulkan_device_get_physical_handle (device),
      .m_pInstance = gulkan_client_get_instance_handle (client),
      .m_pQueue = gulkan_queue_get_handle (queue),
      .m_nQueueFamilyIndex = gulkan_queue_get_family_index (queue),
      .m_nWidth = extent.width,
      .m_nHeight = extent.height,
      .m_nFormat = gulkan_texture_get_format (texture),
      .m_nSampleCount = 1
    };

  struct Texture_t vr_texture =
    {
      .handle = &texture_data,
      .eType = ETextureType_TextureType_Vulkan,
      .eColorSpace = EColorSpace_ColorSpace_Auto
    };

  err = f->overlay->SetOverlayTexture (self->overlay_handle, &vr_texture);

  OVERLAY_CHECK_ERROR ("SetOverlayTexture", err)
  return TRUE;
}

static gboolean
_print_info (GxrOverlay *overlay)
{
  OpenVROverlay *self = OPENVR_OVERLAY (overlay);

  OpenVRFunctions *f = openvr_get_functions ();
  EVROverlayError err;

  VROverlayTransformType transform_type;
  err = f->overlay->GetOverlayTransformType (self->overlay_handle,
                                            &transform_type);
  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not GetOverlayTransformType: %s\n",
                  f->overlay->GetOverlayErrorNameFromEnum (err));
      return FALSE;
    }

  switch (transform_type)
    {
    case VROverlayTransformType_VROverlayTransform_Absolute:
      g_print ("VROverlayTransform_Absolute\n");
      break;
    case VROverlayTransformType_VROverlayTransform_TrackedDeviceRelative:
      g_print ("VROverlayTransform_TrackedDeviceRelative\n");
      break;
    case VROverlayTransformType_VROverlayTransform_SystemOverlay:
      g_print ("VROverlayTransform_SystemOverlay\n");
      break;
    case VROverlayTransformType_VROverlayTransform_TrackedComponent:
      g_print ("VROverlayTransform_TrackedComponent\n");
      break;
#if (OPENVR_VERSION_MINOR >= 9)
    case VROverlayTransformType_VROverlayTransform_Invalid:
      g_print ("VROverlayTransform_Invalid\n");
      break;
    case VROverlayTransformType_VROverlayTransform_Cursor:
      g_print ("VROverlayTransform_Cursor\n");
      break;
    case VROverlayTransformType_VROverlayTransform_DashboardTab:
      g_print ("VROverlayTransform_DashboardTab\n");
      break;
    case VROverlayTransformType_VROverlayTransform_DashboardThumb:
      g_print ("VROverlayTransform_DashboardThumb\n");
      break;
#endif
#if (OPENVR_VERSION_MINOR >= 10)
    case VROverlayTransformType_VROverlayTransform_Mountable:
      g_print ("VROverlayTransform_Mountable\n");
      break;
#endif
#if (OPENVR_VERSION_MINOR >= 16)
    case VROverlayTransformType_VROverlayTransform_Projection:
      g_print ("VROverlayTransform_Projection\n");
      break;
#endif
    }

  TrackingUniverseOrigin tracking_origin;
  HmdMatrix34_t transform;

  err = f->overlay->GetOverlayTransformAbsolute (
    self->overlay_handle,
    &tracking_origin,
    &transform);
  if (err != EVROverlayError_VROverlayError_None)
    {
      g_printerr ("Could not GetOverlayTransformAbsolute: %s\n",
                  f->overlay->GetOverlayErrorNameFromEnum (err));
      return FALSE;
    }

  switch (tracking_origin)
    {
    case ETrackingUniverseOrigin_TrackingUniverseSeated:
      g_print ("ETrackingUniverseOrigin_TrackingUniverseSeated\n");
      break;
    case ETrackingUniverseOrigin_TrackingUniverseStanding:
      g_print ("ETrackingUniverseOrigin_TrackingUniverseStanding\n");
      break;
    case ETrackingUniverseOrigin_TrackingUniverseRawAndUncalibrated:
      g_print ("ETrackingUniverseOrigin_TrackingUniverseRawAndUncalibrated\n");
      break;
    }

  openvr_math_print_matrix34 (transform);

  return TRUE;
}

static void
openvr_overlay_class_init (OpenVROverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;

  GxrOverlayClass *parent_class = GXR_OVERLAY_CLASS (klass);
  parent_class->poll_event = _poll_event;
  parent_class->set_mouse_scale = _set_mouse_scale;
  parent_class->is_visible = _is_visible;
  parent_class->thumbnail_is_visible = _thumbnail_is_visible;
  parent_class->show = _show;
  parent_class->hide = _hide;
  parent_class->set_sort_order = _set_sort_order;
  parent_class->clear_texture = _clear_texture;
  parent_class->get_color = _get_color;
  parent_class->set_color = _set_color;
  parent_class->set_alpha = _set_alpha;
  parent_class->set_width_meters = _set_width_meters;
  parent_class->set_transform_absolute = _set_transform_absolute;
  parent_class->set_raw = _set_raw;
  parent_class->get_size_pixels = _get_size_pixels;
  parent_class->get_width_meters = _get_width_meters;
  parent_class->enable_mouse_input = _enable_mouse_input;
  parent_class->get_transform_absolute = _get_transform_absolute;
  parent_class->show_keyboard = _show_keyboard;
  parent_class->set_keyboard_position = _set_keyboard_position;
  parent_class->submit_texture = _submit_texture;
  parent_class->print_info = _print_info;
  parent_class->set_flip_y = _set_flip_y;
}
