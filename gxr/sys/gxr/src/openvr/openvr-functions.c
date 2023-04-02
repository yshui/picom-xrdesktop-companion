/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "openvr-functions.h"
#include <glib/gprintf.h>

#define INIT_FN_TABLE(target, type) \
{ \
  intptr_t ptr = 0; \
  gboolean ret = _init_fn_table (IVR##type##_Version, &ptr); \
  if (!ret || ptr == 0) \
    return NULL; \
  target = (struct VR_IVR##type##_FnTable*) ptr; \
}

G_DEFINE_TYPE (OpenVRFunctions, openvr_functions, G_TYPE_OBJECT)

static void
openvr_functions_init (OpenVRFunctions *self)
{
  self->system = NULL;
  self->overlay = NULL;
  self->compositor = NULL;
  self->input = NULL;
  self->model = NULL;
  self->applications = NULL;
}

static void
_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (openvr_functions_parent_class)->finalize (gobject);
}

static gboolean
_init_fn_table (const char *type, intptr_t *ret)
{
  EVRInitError error;
  char fn_table_name[128];
  g_sprintf (fn_table_name, "FnTable:%s", type);

  *ret = VR_GetGenericInterface (fn_table_name, &error);

  if (error != EVRInitError_VRInitError_None)
    {
      g_printerr ("VR_GetGenericInterface returned error %s: %s\n",
                  VR_GetVRInitErrorAsSymbol (error),
                  VR_GetVRInitErrorAsEnglishDescription (error));
      return FALSE;
    }

  return TRUE;
}

OpenVRFunctions *
openvr_functions_new (void)
{
  OpenVRFunctions *self =
    (OpenVRFunctions*) g_object_new (OPENVR_TYPE_FUNCTIONS, 0);

  INIT_FN_TABLE (self->system, System)
  INIT_FN_TABLE (self->overlay, Overlay)
  INIT_FN_TABLE (self->compositor, Compositor)
  INIT_FN_TABLE (self->input, Input)
  INIT_FN_TABLE (self->model, RenderModels)
  INIT_FN_TABLE (self->applications, Applications)

  return self;
}

static void
openvr_functions_class_init (OpenVRFunctionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = _finalize;
}

gboolean
openvr_functions_is_valid (OpenVRFunctions *self)
{
  return self->system != NULL
    && self->overlay != NULL
    && self->compositor != NULL
    && self->input != NULL
    && self->model != NULL
    && self->applications != NULL;
}
