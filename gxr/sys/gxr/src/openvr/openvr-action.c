/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include "gxr-types.h"
#include "gxr-io.h"

#include "openvr-wrapper.h"
#include "openvr-math.h"
#include "openvr-action.h"
#include "openvr-action-set.h"
#include "openvr-context.h"
#include "openvr-compositor.h"

#include "gxr-context.h"
#include "gxr-controller.h"

/* openvr k_unMaxTrackedDeviceCount */
#define MAX_DEVICE_COUNT 64

#define INVALID_DEVICE_PATH UINT64_MAX

struct _OpenVRAction
{
  GxrAction parent;
  GxrContext *context;

  VRActionHandle_t handle;

  gboolean controller_update_required;

  /* Only used for DIGITAL_FROM_FLOAT */
  float threshold;
  float last_float[MAX_DEVICE_COUNT];
  gboolean last_bool[MAX_DEVICE_COUNT];
  GxrAction *haptic_action;
};

G_DEFINE_TYPE (OpenVRAction, openvr_action, GXR_TYPE_ACTION)

gboolean
openvr_action_load_handle (OpenVRAction *self,
                           char         *url);

static void
openvr_action_init (OpenVRAction *self)
{
  self->handle = k_ulInvalidActionHandle;
  for (int i = 0; i < MAX_DEVICE_COUNT; i++)
    {
      self->last_float[i] = 0.0f;
      self->last_bool[i] = FALSE;
    }
  self->haptic_action = NULL;
  self->context = NULL;
  self->controller_update_required = FALSE;
}

void
openvr_action_update_controllers (OpenVRAction *self)
{
  OpenVRFunctions *f = openvr_get_functions ();

  GxrActionSet *action_set = gxr_action_get_action_set (GXR_ACTION (self));

  VRActionSetHandle_t actionset_handle =
    openvr_action_set_get_handle (OPENVR_ACTION_SET (action_set));

  VRInputValueHandle_t *origin_handles =
    g_malloc (sizeof (VRInputValueHandle_t) * k_unMaxActionOriginCount);
  EVRInputError err =
    f->input->GetActionOrigins (actionset_handle, self->handle,
                                origin_handles, k_unMaxActionOriginCount);

  if (err != EVRInputError_VRInputError_None)
    {
      g_debug ("GetActionOrigins for %s failed, retrying later...\n",
               gxr_action_get_url (GXR_ACTION (self)));
      self->controller_update_required = TRUE;
      g_free (origin_handles);
      return;
    }

  if (self->controller_update_required)
    g_debug ("GetActionOrigins for %s succeeded now\n",
             gxr_action_get_url (GXR_ACTION (self)));
  self->controller_update_required = FALSE;

  int origin_count = -1;
  while (origin_handles[++origin_count] != k_ulInvalidInputValueHandle);

  // g_print ("Action %s has %d origins\n", self->url, origin_count);

  for (int i = 0; i < origin_count; i++)
    {
      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (origin_handles[i],
                                                 &origin_info,
                                                  sizeof (origin_info));
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("GetOriginTrackedDeviceInfo for %s failed\n",
                      gxr_action_get_url (GXR_ACTION (self)));
          g_free (origin_handles);
          return;
        }

      TrackedDeviceIndex_t device_index  = origin_info.trackedDeviceIndex;

      if (!f->system->IsTrackedDeviceConnected (device_index))
        {
          g_debug ("Skipping unconnected device %d\n", device_index);
          continue;
        }

      if (f->system->GetTrackedDeviceClass (device_index) !=
        ETrackedDeviceClass_TrackedDeviceClass_Controller)
        {
          g_debug ("Skipping device %d, not a controller\n", device_index);
          continue;
        }

      GxrDeviceManager *device_manager = gxr_context_get_device_manager (self->context);
      if (!gxr_device_manager_get (device_manager, device_index))
        {
          gxr_device_manager_add (device_manager, self->context,
                                  device_index, TRUE);


          char *origin_name = g_malloc (sizeof (char) * k_unMaxActionNameLength);
          f->input->GetOriginLocalizedName (origin_handles[i], origin_name,
                                            k_unMaxActionNameLength,
                                            EVRInputStringBits_VRInputString_All);
          g_print ("Added controller for origin %s for action %s\n", origin_name,
                   gxr_action_get_url (GXR_ACTION (self)));
          g_free (origin_name);
        }
    }

  g_free (origin_handles);
}

/* TODO: maybe there is a better way */
static VRInputValueHandle_t
_index_to_device_path (OpenVRAction *self, TrackedDeviceIndex_t index)
{
  OpenVRFunctions *f = openvr_get_functions ();

  GxrActionSet *action_set = gxr_action_get_action_set (GXR_ACTION (self));

  VRActionSetHandle_t actionset_handle =
    openvr_action_set_get_handle (OPENVR_ACTION_SET (action_set));

  VRInputValueHandle_t *origin_handles =
  g_malloc (sizeof (VRInputValueHandle_t) * k_unMaxActionOriginCount);
  EVRInputError err =
    f->input->GetActionOrigins (actionset_handle, self->handle,
                                origin_handles, k_unMaxActionOriginCount);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("GetActionOrigins for %s failed, retrying later...\n",
                  gxr_action_get_url (GXR_ACTION (self)));
      g_free (origin_handles);
      return INVALID_DEVICE_PATH;
    }

  int origin_count = -1;
  while (origin_handles[++origin_count] != k_ulInvalidInputValueHandle);

  // g_print ("Action %s has %d origins\n", self->url, origin_count);

  for (int i = 0; i < origin_count; i++)
    {
      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (origin_handles[i],
                                                  &origin_info,
                                                  sizeof (origin_info));
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("GetOriginTrackedDeviceInfo for %s failed\n",
                      gxr_action_get_url (GXR_ACTION (self)));
          continue;
        }

      if (origin_info.trackedDeviceIndex == index)
        {
          g_free (origin_handles);
          return origin_info.devicePath;
        }
    }

  g_free (origin_handles);
  return INVALID_DEVICE_PATH;
}

static OpenVRAction *
openvr_action_new (void)
{
  return (OpenVRAction*) g_object_new (OPENVR_TYPE_ACTION, 0);
}

static gboolean
_poll (GxrAction *action);

OpenVRAction *
openvr_action_new_from_type_url (GxrContext *context, GxrActionSet *action_set,
                                 GxrActionType type, char *url)
{
  OpenVRAction *self = openvr_action_new ();

  self->context = context;
  gxr_action_set_action_type (GXR_ACTION (self), type);
  gxr_action_set_url (GXR_ACTION (self), g_strdup (url));
  gxr_action_set_action_set(GXR_ACTION (self), action_set);

  if (!openvr_action_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }

  return self;
}

gboolean
openvr_action_load_handle (OpenVRAction *self,
                           char         *url)
{
  OpenVRFunctions *f = openvr_get_functions ();
  EVRInputError err = f->input->GetActionHandle (url, &self->handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetActionHandle: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }
  return TRUE;
}

static gboolean
_action_poll_digital (OpenVRAction *self)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputDigitalActionData_t data;

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  EVRInputError err;
  for(GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = l->data;

      TrackedDeviceIndex_t index =
        (TrackedDeviceIndex_t) gxr_device_get_handle (GXR_DEVICE (controller));

      VRActionHandle_t input_handle = _index_to_device_path (self, index);

      err = f->input->GetDigitalActionData (self->handle, &data,
                                            sizeof(data), input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetDigitalActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
                                                 &origin_info,
                                                  sizeof (origin_info));

      if (err != EVRInputError_VRInputError_None)
        {
          /* controller is not active, but might be active later */
          if (err == EVRInputError_VRInputError_InvalidHandle)
            continue;

          g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
      event->controller = controller;
      event->active = data.bActive;
      event->state = data.bState;
      event->changed = data.bChanged;
      event->time = data.fUpdateTime;

      gxr_action_emit_digital (GXR_ACTION (self), event);
    }
  return TRUE;
}

static gboolean
_threshold_passed (float threshold, float last, float current)
{
  return
    (last < threshold && current >= threshold) ||
    (last >= threshold && current <= threshold);
}

static gboolean
_action_poll_digital_from_float (OpenVRAction *self)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputAnalogActionData_t data;

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  EVRInputError err;
  for(GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = l->data;

      TrackedDeviceIndex_t index =
        (TrackedDeviceIndex_t) gxr_device_get_handle (GXR_DEVICE (controller));
      VRInputValueHandle_t input_handle = _index_to_device_path (self, index);
      err = f->input->GetAnalogActionData (self->handle, &data,
                                            sizeof(data), input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetAnalogActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
                                                  &origin_info,
                                                  sizeof (origin_info));

      if (err != EVRInputError_VRInputError_None)
        {
          /* controller is not active, but might be active later */
          if (err == EVRInputError_VRInputError_InvalidHandle)
            continue;

          g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }


      if (self->haptic_action &&
          _threshold_passed (self->threshold,
                              self->last_float[origin_info.trackedDeviceIndex],
                              data.x))
        {
          g_debug ("Threshold %f passed, triggering haptic\n", self->threshold);
          gxr_action_trigger_haptic (GXR_ACTION (self->haptic_action),
                                      0.f, 0.03f, 50.f, 0.4f,
                                      origin_info.devicePath);
        }

      gboolean currentState = data.x >= self->threshold;

      GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
      event->controller = controller;
      event->active = data.bActive;
      event->state = currentState;
      event->changed =
        currentState != self->last_bool[origin_info.trackedDeviceIndex];
      event->time = data.fUpdateTime;

      gxr_action_emit_digital (GXR_ACTION (self), event);

      self->last_float[origin_info.trackedDeviceIndex] = data.x;
      self->last_bool[origin_info.trackedDeviceIndex] = currentState;
    }

  return TRUE;
}

static gboolean
_action_poll_analog (OpenVRAction *self)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputAnalogActionData_t data;

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  EVRInputError err;
  for(GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = l->data;


      TrackedDeviceIndex_t index =
        (TrackedDeviceIndex_t) gxr_device_get_handle (GXR_DEVICE (controller));
      VRInputValueHandle_t input_handle = _index_to_device_path (self, index);
      err = f->input->GetAnalogActionData (self->handle, &data,
                                           sizeof(data), input_handle);

      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetAnalogActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      InputOriginInfo_t origin_info;
      err = f->input->GetOriginTrackedDeviceInfo (data.activeOrigin,
                                                 &origin_info,
                                                  sizeof (origin_info));

      if (err != EVRInputError_VRInputError_None)
        {
          /* controller is not active, but might be active later */
          if (err == EVRInputError_VRInputError_InvalidHandle)
            continue;

          g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      GxrAnalogEvent *event = g_malloc (sizeof (GxrAnalogEvent));
      event->active = data.bActive;
      event->controller = controller;
      graphene_vec3_init (&event->state, data.x, data.y, data.z);
      graphene_vec3_init (&event->delta, data.deltaX, data.deltaY, data.deltaZ);
      event->time = data.fUpdateTime;

      gxr_action_emit_analog (GXR_ACTION (self), event);
    }

  return TRUE;
}

static gboolean
_emit_pose_event (OpenVRAction          *self,
                  InputPoseActionData_t *data)
{
  OpenVRFunctions *f = openvr_get_functions ();

  InputOriginInfo_t origin_info;
  EVRInputError err;
  err = f->input->GetOriginTrackedDeviceInfo (data->activeOrigin,
                                             &origin_info,
                                              sizeof (origin_info));
  if (err != EVRInputError_VRInputError_None)
    {
      /* controller is not active, but might be active later */
      if (err == EVRInputError_VRInputError_InvalidHandle)
        return TRUE;

      g_printerr ("ERROR: GetOriginTrackedDeviceInfo: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GxrDevice *controller =
    gxr_device_manager_get (dm, origin_info.trackedDeviceIndex);
  if (!controller || !gxr_device_is_controller (controller))
    return TRUE;

  GxrPoseEvent *event = g_malloc (sizeof (GxrPoseEvent));
  event->active = data->bActive;
  event->controller = GXR_CONTROLLER (controller);
  openvr_math_matrix34_to_graphene (&data->pose.mDeviceToAbsoluteTracking,
                                    &event->pose);
  graphene_vec3_init_from_float (&event->velocity,
                                 data->pose.vVelocity.v);
  graphene_vec3_init_from_float (&event->angular_velocity,
                                 data->pose.vAngularVelocity.v);
  event->valid = data->pose.bPoseIsValid;
  event->device_connected = data->pose.bDeviceIsConnected;

  gxr_action_emit_pose (GXR_ACTION (self), event);

  return TRUE;
}

static gboolean
_action_poll_pose_secs_from_now (OpenVRAction *self,
                                 float         secs)
{
  OpenVRFunctions *f = openvr_get_functions ();

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  EVRInputError err;
  for(GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = l->data;

      TrackedDeviceIndex_t index =
        (TrackedDeviceIndex_t) gxr_device_get_handle (GXR_DEVICE (controller));
      VRInputValueHandle_t input_handle = _index_to_device_path (self, index);

      InputPoseActionData_t data;

      enum ETrackingUniverseOrigin origin = f->compositor->GetTrackingSpace ();

      err = f->input->GetPoseActionDataRelativeToNow (self->handle,
                                                      origin,
                                                      secs,
                                                     &data,
                                                      sizeof(data),
                                                     input_handle);
      if (err != EVRInputError_VRInputError_None)
        {
          g_printerr ("ERROR: GetPoseActionData: %s\n",
                      openvr_input_error_string (err));
          return FALSE;
        }

      if (!_emit_pose_event (self, &data))
        return FALSE;

    }
  return TRUE;
}

static gboolean
_poll (GxrAction *action)
{
  OpenVRAction *self = OPENVR_ACTION (action);

  if (self->controller_update_required)
    openvr_action_update_controllers (self);

  GxrActionType type = gxr_action_get_action_type (action);
  switch (type)
    {
    case GXR_ACTION_DIGITAL:
      return _action_poll_digital (self);
    case GXR_ACTION_DIGITAL_FROM_FLOAT:
      return _action_poll_digital_from_float (self);
    case GXR_ACTION_FLOAT:
    case GXR_ACTION_VEC2F:
      return _action_poll_analog (self);
    case GXR_ACTION_POSE:
      return _action_poll_pose_secs_from_now (self, 0);
    default:
      g_printerr ("Uknown action type %d\n", type);
      return FALSE;
    }
}

static gboolean
_trigger_haptic (GxrAction *action,
                 float start_seconds_from_now,
                 float duration_seconds,
                 float frequency,
                 float amplitude,
                 guint64 controller_handle)
{
  OpenVRAction *self = OPENVR_ACTION (action);

  OpenVRFunctions *f = openvr_get_functions ();

  EVRInputError err;
  err = f->input->TriggerHapticVibrationAction (
    self->handle,
    start_seconds_from_now,
    duration_seconds,
    frequency,
    amplitude,
    controller_handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: TriggerHapticVibrationAction: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

static void
_finalize (GObject *gobject)
{
  OpenVRAction *self = OPENVR_ACTION (gobject);
  if (self->haptic_action)
    g_object_unref (self->haptic_action);
}

static void
_set_digital_from_float_threshold (GxrAction *action,
                                   float      threshold)
{
  OpenVRAction *self = OPENVR_ACTION (action);
  self->threshold = threshold;
}

static void
_set_digital_from_float_haptic (GxrAction *action,
                                GxrAction *haptic_action)
{
  OpenVRAction *self = OPENVR_ACTION (action);
  self->haptic_action = haptic_action;
}

static void
openvr_action_class_init (OpenVRActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize =_finalize;

  GxrActionClass *gxr_action_class = GXR_ACTION_CLASS (klass);
  gxr_action_class->poll = _poll;
  gxr_action_class->trigger_haptic = _trigger_haptic;
  gxr_action_class->set_digital_from_float_threshold =
    _set_digital_from_float_threshold;
  gxr_action_class->set_digital_from_float_haptic =
    _set_digital_from_float_haptic;
}
