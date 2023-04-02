/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_OVERLAY_WINDOW_PRIVATE_H_
#define XRD_OVERLAY_WINDOW_PRIVATE_H_

#include "xrd-overlay-window.h"

G_BEGIN_DECLS

XrdOverlayWindow *
xrd_overlay_window_new_from_meters (GxrContext  *context,
                                    const gchar *title,
                                    float        width,
                                    float        height,
                                    float        ppm);

XrdOverlayWindow *
xrd_overlay_window_new_from_data (GxrContext    *context,
                                  XrdWindowData *data);

XrdOverlayWindow *
xrd_overlay_window_new_from_pixels (GxrContext  *context,
                                    const gchar *title,
                                    uint32_t     width,
                                    uint32_t     height,
                                    float        ppm);

XrdOverlayWindow *
xrd_overlay_window_new_from_native (GxrContext  *context,
                                    const gchar *title,
                                    gpointer     native,
                                    uint32_t     width_pixels,
                                    uint32_t     height_pixels,
                                    float        ppm);


G_END_DECLS

#endif /* XRD_OVERLAY_WINDOW_PRIVATE_H_ */
