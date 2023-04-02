/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_CONTEXT_H_
#define GXR_OPENVR_CONTEXT_H_

#include <glib-object.h>
#include <graphene.h>

#include <stdint.h>

#include "gxr-enums.h"
#include "gxr-context.h"

#include "openvr-wrapper.h"
#include "openvr-functions.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_CONTEXT openvr_context_get_type()
G_DECLARE_FINAL_TYPE (OpenVRContext, openvr_context,
                      OPENVR, CONTEXT, GxrContext)
OpenVRContext *openvr_context_new (void);

gboolean
openvr_context_is_installed (void);

gboolean
openvr_context_is_hmd_present (void);

OpenVRFunctions*
openvr_get_functions (void);

G_END_DECLS

#endif /* GXR_OPENVR_CONTEXT_H_ */
