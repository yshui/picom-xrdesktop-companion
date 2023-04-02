/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_CLIENT_PRIVATE_H_
#define XRD_CLIENT_PRIVATE_H_

#include "xrd-client.h"

G_BEGIN_DECLS

void
xrd_client_set_context (XrdClient *self, GxrContext *context);


void
xrd_client_emit_keyboard_press (XrdClient *self,
                                GdkEventKey *event);

void
xrd_client_emit_click (XrdClient *self,
                       XrdClickEvent *event);

void
xrd_client_emit_move_cursor (XrdClient *self,
                             XrdMoveCursorEvent *event);

void
xrd_client_emit_system_quit (XrdClient *self,
                             GxrQuitEvent *event);

void
xrd_client_init_input_callbacks (XrdClient *self);

void
xrd_client_set_upload_layout (XrdClient *self, VkImageLayout layout);

void
xrd_client_set_desktop_cursor (XrdClient        *self,
                               XrdDesktopCursor *cursor);

XrdWindow *
xrd_client_window_new_from_data (XrdClient     *self,
                                 XrdWindowData *data);

XrdWindow *
xrd_client_window_new_from_meters (XrdClient  *self,
                                   const char *title,
                                   float       width,
                                   float       height,
                                   float       ppm);

XrdWindow *
xrd_client_window_new_from_pixels (XrdClient  *self,
                                   const char *title,
                                   uint32_t    width,
                                   uint32_t    height,
                                   float       ppm);

XrdWindow *
xrd_client_window_new_from_native (XrdClient   *self,
                                   const gchar *title,
                                   gpointer     native,
                                   uint32_t     width_pixels,
                                   uint32_t     height_pixels,
                                   float        ppm);

G_END_DECLS

#endif /* XRD_CLIENT_PRIVATE_H_ */
