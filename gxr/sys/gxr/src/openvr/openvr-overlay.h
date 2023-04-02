/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENVR_OVERLAY_H_
#define GXR_OPENVR_OVERLAY_H_

#include "gxr-overlay.h"

G_BEGIN_DECLS

#define OPENVR_TYPE_OVERLAY openvr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenVROverlay, openvr_overlay, OPENVR, OVERLAY, GxrOverlay)
OpenVROverlay *openvr_overlay_new (gchar* key);

G_END_DECLS

#endif /* GXR_OPENVR_OVERLAY_H_ */
