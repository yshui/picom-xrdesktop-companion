/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_FUNCTIONS_H_
#define GXR_OPENVR_FUNCTIONS_H_

#include <glib.h>
#include <glib-object.h>
#include "openvr-wrapper.h"

G_BEGIN_DECLS

struct _OpenVRFunctions
{
  GObject parent;

  struct VR_IVRSystem_FnTable *system;
  struct VR_IVROverlay_FnTable *overlay;
  struct VR_IVRCompositor_FnTable *compositor;
  struct VR_IVRInput_FnTable *input;
  struct VR_IVRRenderModels_FnTable *model;
  struct VR_IVRApplications_FnTable *applications;
};

#define OPENVR_TYPE_FUNCTIONS openvr_functions_get_type()
G_DECLARE_FINAL_TYPE (OpenVRFunctions, openvr_functions,
                      OPENVR, FUNCTIONS, GObject)

OpenVRFunctions*
openvr_functions_new (void);

gboolean
openvr_functions_is_valid (OpenVRFunctions *self);

G_END_DECLS

#endif /* GXR_OPENVR_FUNCTIONS_H_ */
