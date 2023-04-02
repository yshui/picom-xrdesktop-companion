/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OVERLAY_PRIVATE_H_
#define GXR_OVERLAY_PRIVATE_H_

#include "gxr-overlay.h"

G_BEGIN_DECLS

/* Signal emission */
void
gxr_overlay_emit_motion_notify (GxrOverlay *self, GdkEvent *event);

void
gxr_overlay_emit_button_press (GxrOverlay *self, GdkEvent *event);

void
gxr_overlay_emit_button_release (GxrOverlay *self, GdkEvent *event);

void
gxr_overlay_emit_show (GxrOverlay *self);

void
gxr_overlay_emit_hide (GxrOverlay *self);

void
gxr_overlay_emit_destroy (GxrOverlay *self);

void
gxr_overlay_emit_keyboard_press (GxrOverlay *self, GdkEvent *event);

void
gxr_overlay_emit_keyboard_close (GxrOverlay *self);

G_END_DECLS

#endif /* GXR_OVERLAY_PRIVATE_H_ */
