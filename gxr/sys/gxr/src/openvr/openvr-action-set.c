/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-action-set.h"
#include "openvr-action-set.h"

#include "openvr-context.h"
#include "openvr-action.h"

struct _OpenVRActionSet
{
  GxrActionSet parent;
  guint64 binding_loaded_source;
  GxrContext *context;
  VRActionSetHandle_t handle;
};

G_DEFINE_TYPE (OpenVRActionSet, openvr_action_set, GXR_TYPE_ACTION_SET)

VRActionSetHandle_t
openvr_action_set_get_handle (OpenVRActionSet *self)
{
  return self->handle;
}

static void
openvr_action_set_finalize (GObject *gobject);

static gboolean
_update (GxrActionSet **sets, uint32_t count);

static void
openvr_action_set_init (OpenVRActionSet *self)
{
  self->handle = k_ulInvalidActionSetHandle;
  self->binding_loaded_source = 0;
  self->context = NULL;
}

void
openvr_action_set_update_controllers (OpenVRActionSet *self)
{
  GSList *actions = gxr_action_set_get_actions (GXR_ACTION_SET (self));
  for (GSList *l = actions; l != NULL; l = l->next)
  {
    OpenVRAction *action = OPENVR_ACTION (l->data);
    openvr_action_update_controllers (action);
  }
}


static gboolean
_load_handle (OpenVRActionSet *self,
              gchar           *url)
{
  EVRInputError err;

  OpenVRFunctions *f = openvr_get_functions ();

  err = f->input->GetActionSetHandle (url, &self->handle);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: GetActionSetHandle: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

static void
_binding_loaded_cb (void          *context,
                    gpointer       _self)
{
  (void) context;
  OpenVRActionSet *self = OPENVR_ACTION_SET (_self);
  openvr_action_set_update_controllers (self);
}

OpenVRActionSet *
openvr_action_set_new_from_url (OpenVRContext *context, gchar *url)
{
  OpenVRActionSet *self = openvr_action_set_new (context);
  if (!_load_handle (self, url))
    {
      g_object_unref (self);
      self = NULL;
    }
  return self;
}

OpenVRActionSet *
openvr_action_set_new (OpenVRContext *context)
{
  OpenVRActionSet *self = (OpenVRActionSet*) g_object_new (OPENVR_TYPE_ACTION_SET, 0);
  self->context = GXR_CONTEXT (context);

  self->binding_loaded_source =
    g_signal_connect (context, "binding-loaded-event",
                      (GCallback) _binding_loaded_cb, self);

  return self;
}

static void
openvr_action_set_finalize (GObject *gobject)
{
  OpenVRActionSet *self = OPENVR_ACTION_SET (gobject);
  if (self->binding_loaded_source != 0)
    g_signal_handler_disconnect (self->context, self->binding_loaded_source);

  G_OBJECT_CLASS (openvr_action_set_parent_class)->finalize (gobject);
}

static gboolean
_update (GxrActionSet **sets, uint32_t count)
{
  struct VRActiveActionSet_t *active_action_sets =
    g_malloc (sizeof (struct VRActiveActionSet_t) * count);

  for (uint32_t i = 0; i < count; i++)
    {
      active_action_sets[i].ulActionSet = OPENVR_ACTION_SET (sets[i])->handle;
      active_action_sets[i].ulRestrictedToDevice = 0;
      active_action_sets[i].ulSecondaryActionSet = 0;
      active_action_sets[i].unPadding = 0;
      active_action_sets[i].nPriority = 0;
    }

  OpenVRFunctions *f = openvr_get_functions ();

  EVRInputError err;
  err = f->input->UpdateActionState (active_action_sets,
                                     sizeof (struct VRActiveActionSet_t),
                                     count);

  g_free (active_action_sets);

  if (err != EVRInputError_VRInputError_None)
    {
      g_printerr ("ERROR: UpdateActionState: %s\n",
                  openvr_input_error_string (err));
      return FALSE;
    }

  return TRUE;
}

#define ENUM_TO_STR(r) case r: return #r

const gchar*
openvr_input_error_string (EVRInputError err)
{
  switch (err)
    {
      ENUM_TO_STR(EVRInputError_VRInputError_None);
      ENUM_TO_STR(EVRInputError_VRInputError_NameNotFound);
      ENUM_TO_STR(EVRInputError_VRInputError_WrongType);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidHandle);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidParam);
      ENUM_TO_STR(EVRInputError_VRInputError_NoSteam);
      ENUM_TO_STR(EVRInputError_VRInputError_MaxCapacityReached);
      ENUM_TO_STR(EVRInputError_VRInputError_IPCError);
      ENUM_TO_STR(EVRInputError_VRInputError_NoActiveActionSet);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidDevice);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidSkeleton);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidBoneCount);
      ENUM_TO_STR(EVRInputError_VRInputError_InvalidCompressedData);
      ENUM_TO_STR(EVRInputError_VRInputError_NoData);
      ENUM_TO_STR(EVRInputError_VRInputError_BufferTooSmall);
      ENUM_TO_STR(EVRInputError_VRInputError_MismatchedActionManifest);
      ENUM_TO_STR(EVRInputError_VRInputError_MissingSkeletonData);
      default:
        return "UNKNOWN EVRInputError";
    }
}

static GxrAction*
_create_action (GxrActionSet *self, GxrContext *context,
                GxrActionType type, char *url)
{
  (void) context;
  return (GxrAction*) openvr_action_new_from_type_url (context, self,
                                                       type, url);
}

static void
openvr_action_set_class_init (OpenVRActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = openvr_action_set_finalize;

  GxrActionSetClass *gxr_action_set_class = GXR_ACTION_SET_CLASS (klass);
  gxr_action_set_class->update = _update;
  gxr_action_set_class->create_action = _create_action;
}
