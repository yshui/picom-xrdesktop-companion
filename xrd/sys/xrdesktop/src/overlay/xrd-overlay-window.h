/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_OVERLAY_WINDOW_H_
#define XRD_OVERLAY_WINDOW_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include <gulkan.h>
#include <gxr.h>

#include "xrd-window.h"

G_BEGIN_DECLS

#define XRD_TYPE_OVERLAY_WINDOW xrd_overlay_window_get_type()
G_DECLARE_FINAL_TYPE (XrdOverlayWindow, xrd_overlay_window,
                      XRD, OVERLAY_WINDOW, GObject)

G_END_DECLS

#endif /* XRD_OVERLAY_WINDOW_H_ */
