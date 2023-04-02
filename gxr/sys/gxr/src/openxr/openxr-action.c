/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include "gxr-types.h"

#include "openxr-action.h"
#include "openxr-context.h"
#include "openxr-action-set.h"
#include "gxr-controller.h"

#define NUM_HANDS 2
struct _OpenXRAction
{
  GxrAction parent;

  GxrContext *context;
  XrInstance instance;
  XrSession session;

  XrPath hand_paths[NUM_HANDS];

  /* only used when this action is a pose action*/
  XrSpace hand_spaces[NUM_HANDS];
  XrSpace tracked_space;

  char *url;

  XrAction handle;

  // gxr API has delta from last values, but OpenXR does not
  graphene_vec3_t last_vec[NUM_HANDS];

  /* Only used for DIGITAL_FROM_FLOAT */
  float threshold;
  float last_float[NUM_HANDS];
  gboolean last_bool[NUM_HANDS];
  GxrAction *haptic_action;
};

G_DEFINE_TYPE (OpenXRAction, openxr_action, GXR_TYPE_ACTION)

static void
openxr_action_init (OpenXRAction *self)
{
  self->handle = XR_NULL_HANDLE;
  for (int i = 0; i < NUM_HANDS; i++)
    {
      self->hand_spaces[i] = XR_NULL_HANDLE;
      self->last_float[i] = 0.0f;
      self->last_bool[i] = FALSE;

      graphene_vec3_init (&self->last_vec[i], 0, 0, 0);
    }
  self->threshold = 0.0f;
  self->haptic_action = NULL;
}

OpenXRAction *
openxr_action_new (OpenXRContext *context)
{
  OpenXRAction* self = (OpenXRAction*) g_object_new (OPENXR_TYPE_ACTION, 0);

  /* TODO: Handle this more nicely */
  self->context = GXR_CONTEXT (context);
  self->instance = openxr_context_get_openxr_instance (context);
  self->session = openxr_context_get_openxr_session (context);
  self->tracked_space = openxr_context_get_tracked_space (context);

  xrStringToPath(self->instance, "/user/hand/left", &self->hand_paths[0]);
  xrStringToPath(self->instance, "/user/hand/right", &self->hand_paths[1]);

  return self;
}

static gboolean
_url_to_name (char *url, char *name)
{
  char *basename = g_path_get_basename (url);
  if (g_strcmp0 (basename, ".") == 0)
    {
      g_free (basename);
      return FALSE;
    }

  strncpy (name, basename, XR_MAX_ACTION_NAME_SIZE - 1);
  g_free (basename);
  return TRUE;
}

OpenXRAction *
openxr_action_new_from_type_url (OpenXRContext *context,
                                 GxrActionSet *action_set,
                                 GxrActionType type, char *url)
{
  OpenXRAction *self = openxr_action_new (context);
  gxr_action_set_action_type (GXR_ACTION (self), type);
  gxr_action_set_url (GXR_ACTION (self), g_strdup (url));
  gxr_action_set_action_set(GXR_ACTION (self), action_set);
  self->url = g_strdup (url);

  XrActionType action_type;
  switch (type)
  {
    case GXR_ACTION_DIGITAL_FROM_FLOAT:
    case GXR_ACTION_FLOAT:
      action_type = XR_ACTION_TYPE_FLOAT_INPUT;
      break;
    case GXR_ACTION_VEC2F:
      action_type = XR_ACTION_TYPE_VECTOR2F_INPUT;
      break;
    case GXR_ACTION_DIGITAL:
      action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
      break;
    case GXR_ACTION_POSE:
      action_type = XR_ACTION_TYPE_POSE_INPUT;
      break;
    case GXR_ACTION_HAPTIC:
      action_type = XR_ACTION_TYPE_VIBRATION_OUTPUT;
      break;
    default:
      g_printerr ("Unknown action type %d\n", type);
      action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
  }

  XrActionCreateInfo action_info = {
    .type = XR_TYPE_ACTION_CREATE_INFO,
    .next = NULL,
    .actionType = action_type,
    .countSubactionPaths = NUM_HANDS,
    .subactionPaths = self->hand_paths
  };

  /* TODO: proper names, localized name */
  /* TODO: proper names, localized name */
  char name[XR_MAX_ACTION_NAME_SIZE];
  _url_to_name (self->url, name);

  strcpy(action_info.actionName, name);
  strcpy(action_info.localizedActionName, name);

  XrActionSet set = openxr_action_set_get_handle (OPENXR_ACTION_SET (action_set));

  XrResult result = xrCreateAction (set, &action_info, &self->handle);


  if (result != XR_SUCCESS)
    {
      char buffer[XR_MAX_RESULT_STRING_SIZE];
      xrResultToString(self->instance, result, buffer);
      g_printerr ("Failed to create action %s: %s\n", url, buffer);
      g_object_unref (self);
      self = NULL;
    }

  if (action_type == XR_ACTION_TYPE_POSE_INPUT)
    {
      for (int i = 0; i < NUM_HANDS; i++)
        {
          XrActionSpaceCreateInfo action_space_info = {
            .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
            .next = NULL,
            .action = self->handle,
            .poseInActionSpace.orientation.w = 1.f,
            .subactionPath = self->hand_paths[i]
          };

          result = xrCreateActionSpace(self->session,
                                       &action_space_info,
                                       &self->hand_spaces[i]);

          if (result != XR_SUCCESS)
            {
              char buffer[XR_MAX_RESULT_STRING_SIZE];
              xrResultToString(self->instance, result, buffer);
              g_printerr ("Failed to create action space %s: %s\n", url, buffer);
              g_object_unref (self);
              self = NULL;
            }
        }
    }
  return self;
}

static XrPath
_handle_to_subaction (OpenXRAction *self, guint64 handle)
{
  return self->hand_paths[handle];
}

/* equivalent to openvr: "Time relative to now when this event happened" */
static float
_get_time_diff (XrTime xr_time)
{
  (void) xr_time;
  /* TODO: for now pretend everything happens right now */
  return 0;
}

static gboolean
_action_poll_digital (OpenXRAction *self)
{
  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      guint64 controller_handle =
        gxr_device_get_handle (GXR_DEVICE (controller));
      XrPath subaction_path = _handle_to_subaction (self, controller_handle);

      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = subaction_path
      };

      XrActionStateBoolean value = {
        .type = XR_TYPE_ACTION_STATE_BOOLEAN,
        .next = NULL
      };

      XrResult result = xrGetActionStateBoolean(self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll digital action\n");
          continue;
        }

      if (!controller)
        {
          g_print ("Digital without controller\n");
          return TRUE;
        }

      GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
      event->controller = controller;
      event->active = (gboolean)value.isActive;
      event->state = (gboolean)value.currentState;
      event->changed = (gboolean)value.changedSinceLastSync;
      event->time = _get_time_diff (value.lastChangeTime);

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
_action_poll_digital_from_float (OpenXRAction *self)
{
  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      guint64 controller_handle =
        gxr_device_get_handle (GXR_DEVICE (controller));
      XrPath subaction_path = _handle_to_subaction (self, controller_handle);

      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = subaction_path
      };

      XrActionStateFloat value = {
        .type = XR_TYPE_ACTION_STATE_FLOAT,
        .next = NULL
      };

      XrResult result = xrGetActionStateFloat(self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll float action\n");
          continue;
        }

      if (self->haptic_action &&
          _threshold_passed (self->threshold,
                             self->last_float[controller_handle],
                             value.currentState))
        {
          g_debug ("Threshold %f passed, triggering haptic\n", self->threshold);
          gxr_action_trigger_haptic (GXR_ACTION (self->haptic_action),
                                     0.f, 0.03f, 50.f, 0.4f, controller_handle);
        }

      gboolean currentState = value.currentState >= self->threshold;

      GxrDigitalEvent *event = g_malloc (sizeof (GxrDigitalEvent));
      event->controller = controller;
      event->active = (gboolean)value.isActive;
      event->state = (gboolean)currentState;
      event->changed = (gboolean)(value.changedSinceLastSync &&
        currentState != self->last_bool[controller_handle]);
      event->time = _get_time_diff (value.lastChangeTime);

      gxr_action_emit_digital (GXR_ACTION (self), event);
      self->last_float[controller_handle] = value.currentState;
      self->last_bool[controller_handle] = currentState;
    }

  return TRUE;
}

static gboolean
_action_poll_analog (OpenXRAction *self)
{

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      guint64 controller_handle =
        gxr_device_get_handle (GXR_DEVICE (controller));
      XrPath subaction_path = _handle_to_subaction (self, controller_handle);

      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = subaction_path
      };

      XrActionStateFloat value = {
        .type = XR_TYPE_ACTION_STATE_FLOAT,
        .next = NULL
      };

      XrResult result = xrGetActionStateFloat(self->session, &get_info, &value);

      if (result != XR_SUCCESS)
      {
        g_debug ("Failed to poll float action\n");
        continue;
      }

      GxrAnalogEvent *event = g_malloc (sizeof (GxrAnalogEvent));
      event->controller = controller;
      event->active = (gboolean)value.isActive;
      graphene_vec3_init (&event->state, value.currentState, 0, 0);
      graphene_vec3_subtract (&event->state, &self->last_vec[controller_handle], &event->delta);
      event->time = _get_time_diff (value.lastChangeTime);

      gxr_action_emit_analog (GXR_ACTION (self), event);

      graphene_vec3_init_from_vec3 (&self->last_vec[controller_handle], &event->state);
    }

  return TRUE;
}

static gboolean
_action_poll_vec2f (OpenXRAction *self)
{
  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      guint64 controller_handle =
        gxr_device_get_handle (GXR_DEVICE (controller));
      XrPath subaction_path = _handle_to_subaction (self, controller_handle);

      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = subaction_path
      };

      XrActionStateVector2f value = {
        .type = XR_TYPE_ACTION_STATE_VECTOR2F,
        .next = NULL
      };

      XrResult result = xrGetActionStateVector2f(self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll vec2f action\n");
          continue;
        }

      GxrAnalogEvent *event = g_malloc (sizeof (GxrAnalogEvent));
      event->controller = controller;
      event->active = (gboolean)value.isActive;
      graphene_vec3_init (&event->state, value.currentState.x, value.currentState.y, 0);
      graphene_vec3_subtract (&event->state, &self->last_vec[controller_handle], &event->delta);
      event->time = _get_time_diff (value.lastChangeTime);

      gxr_action_emit_analog (GXR_ACTION (self), event);

      graphene_vec3_init_from_vec3 (&self->last_vec[controller_handle], &event->state);
    }

  return TRUE;
}

static gboolean
_space_location_valid (XrSpaceLocation *sl)
{
  return
    /* (sl->locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)    != 0 && */
    (sl->locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
}

static void
_get_model_matrix_from_pose (XrPosef *pose,
                             graphene_matrix_t *mat)
{
  graphene_quaternion_t q;
  graphene_quaternion_init (&q,
                            pose->orientation.x, pose->orientation.y,
                            pose->orientation.z, pose->orientation.w);

  graphene_matrix_init_identity (mat);
  graphene_matrix_rotate_quaternion (mat, &q);
  graphene_point3d_t translation = { .x = pose->position.x, pose->position.y, pose->position.z };
  graphene_matrix_translate(mat, &translation);
}

static gboolean
_action_poll_pose_secs_from_now (OpenXRAction *self,
                                 float         secs)
{
  (void) secs;

  GxrDeviceManager *dm = gxr_context_get_device_manager (self->context);
  GSList *controllers = gxr_device_manager_get_controllers (dm);

  for (GSList *l = controllers; l; l = l->next)
    {
      GxrController *controller = GXR_CONTROLLER (l->data);
      guint64 controller_handle =
        gxr_device_get_handle (GXR_DEVICE (controller));
      XrPath subaction_path = _handle_to_subaction (self, controller_handle);

      XrActionStateGetInfo get_info = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .action = self->handle,
        .subactionPath = subaction_path
      };

      XrActionStatePose value = {
        .type = XR_TYPE_ACTION_STATE_POSE,
        .next = NULL
      };

      XrResult result = xrGetActionStatePose (self->session, &get_info, &value);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll analog action\n");
          continue;
        }

      gboolean spaceLocationValid;
      XrSpaceLocation space_location = {
        .type = XR_TYPE_SPACE_LOCATION,
        .next = NULL
      };

      /* TODO: secs from now ignored, API not appropriate for OpenXR */
      XrTime time = openxr_context_get_predicted_display_time (
        OPENXR_CONTEXT (self->context));


      XrSpace hand_space = self->hand_spaces[controller_handle];
      result = xrLocateSpace (hand_space, self->tracked_space,
                              time, &space_location);

      if (result != XR_SUCCESS)
        {
          g_debug ("Failed to poll hand space location\n");
          continue;
        }

      spaceLocationValid = _space_location_valid (&space_location);

      /*
      g_print("Polled space %s active %d  valid %d, %f %f %f\n", self->url,
              value.isActive, spaceLocationValid,
              space_location.pose.position.x,
              space_location.pose.position.y,
              space_location.pose.position.z
      );
      */

      GxrPoseEvent *event = g_malloc (sizeof (GxrPoseEvent));
      event->active = value.isActive == XR_TRUE;
      event->controller = controller;
      _get_model_matrix_from_pose(&space_location.pose, &event->pose);
      graphene_vec3_init (&event->velocity, 0, 0, 0);
      graphene_vec3_init (&event->angular_velocity, 0, 0, 0);
      event->valid = spaceLocationValid;
      event->device_connected = event->active;

      gxr_action_emit_pose (GXR_ACTION (self), event);
    }

  return TRUE;
}

/* creates controllers with handles 0 for left hand and 1 for right hand.
 * TODO: create controllers dynamically for the inputs bound to this action */
void
openxr_action_update_controllers (OpenXRAction *self)
{
  GxrContext *context = GXR_CONTEXT (self->context);
  GxrDeviceManager *dm = gxr_context_get_device_manager (context);

  for (guint64 i = 0; i < NUM_HANDS; i++)
    {
      guint64 controller_handle = i;

      if (!gxr_device_manager_get (dm, controller_handle))
        {
          gxr_device_manager_add (dm, context, controller_handle, TRUE);

          g_debug ("Added controller %lu from action %s\n",
                   controller_handle, self->url);
        }
    }
}


static gboolean
_poll (GxrAction *action)
{
  OpenXRAction *self = OPENXR_ACTION (action);
  GxrActionType type = gxr_action_get_action_type (action);
  switch (type)
    {
    case GXR_ACTION_DIGITAL:
      return _action_poll_digital (self);
    case GXR_ACTION_DIGITAL_FROM_FLOAT:
      return _action_poll_digital_from_float (self);
    case GXR_ACTION_FLOAT:
      return _action_poll_analog (self);
    case GXR_ACTION_VEC2F:
      return _action_poll_vec2f (self);
    case GXR_ACTION_POSE:
      return _action_poll_pose_secs_from_now (self, 0);
    default:
      g_printerr ("Unknown action type %d\n", type);
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
  (void) start_seconds_from_now;

  OpenXRAction *self = OPENXR_ACTION (action);

  XrTime duration = (XrTime) ((double)duration_seconds * 1000. * 1000. * 1000.);

  // g_debug ("Haptic %f %f Hz, %lu ns\n", amplitude, frequency, duration);

  XrHapticVibration vibration =  {
    .type = XR_TYPE_HAPTIC_VIBRATION,
    .next = NULL,
    .amplitude = amplitude,
    .duration = duration,
    .frequency = frequency
  };

  XrHapticActionInfo hapticActionInfo = {
    .type = XR_TYPE_HAPTIC_ACTION_INFO,
    .next = NULL,
    .action = self->handle,
    .subactionPath = self->hand_paths[controller_handle]
  };

  XrResult result =
    xrApplyHapticFeedback(self->session, &hapticActionInfo,
                          (const XrHapticBaseHeader*)&vibration);

  return result == XR_SUCCESS;
}

char *
openxr_action_get_url (OpenXRAction *self)
{
  return self->url;
}

XrAction
openxr_action_get_handle (OpenXRAction *self)
{
  return self->handle;
}

static void
_finalize (GObject *gobject)
{
  OpenXRAction *self = OPENXR_ACTION (gobject);
  if (self->haptic_action)
    g_object_unref (self->haptic_action);
  g_free (self->url);
}

static void
_set_digital_from_float_threshold (GxrAction *action,
                                   float      threshold)
{
  OpenXRAction *self = OPENXR_ACTION (action);
  self->threshold = threshold;
}

static void
_set_digital_from_float_haptic (GxrAction *action,
                                GxrAction *haptic_action)
{
  OpenXRAction *self = OPENXR_ACTION (action);
  self->haptic_action = haptic_action;
}

static void
openxr_action_class_init (OpenXRActionClass *klass)
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
