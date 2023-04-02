/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_OVERLAY_H_
#define GXR_OPENXR_OVERLAY_H_

#include "gxr-overlay.h"

G_BEGIN_DECLS

#define OPENXR_TYPE_OVERLAY openxr_overlay_get_type()
G_DECLARE_FINAL_TYPE (OpenXROverlay, openxr_overlay, OPENXR, OVERLAY, GxrOverlay)
OpenXROverlay *openxr_overlay_new (void);

G_END_DECLS

#endif /* GXR_OPENXR_OVERLAY_H_ */
