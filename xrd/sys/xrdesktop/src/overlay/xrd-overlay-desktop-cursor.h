/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_OVERLAY_DESKTOP_CURSOR_H_
#define XRD_OVERLAY_DESKTOP_CURSOR_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include "xrd-overlay-window.h"

G_BEGIN_DECLS

#define XRD_TYPE_OVERLAY_DESKTOP_CURSOR xrd_overlay_desktop_cursor_get_type()
G_DECLARE_FINAL_TYPE (XrdOverlayDesktopCursor, xrd_overlay_desktop_cursor, XRD,
                      OVERLAY_DESKTOP_CURSOR, GObject)

struct _XrdOverlayDesktopCursor;

XrdOverlayDesktopCursor *
xrd_overlay_desktop_cursor_new (GxrContext *context);

G_END_DECLS

#endif /* XRD_OVERLAY_DESKTOP_CURSOR_H_ */
