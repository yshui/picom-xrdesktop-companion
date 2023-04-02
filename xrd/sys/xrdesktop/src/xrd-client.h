/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_CLIENT_H_
#define XRD_CLIENT_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

#include <gxr.h>

#include "xrd-window.h"
#include "xrd-input-synth.h"
#include "xrd-window-manager.h"
#include "xrd-desktop-cursor.h"
#include "xrd-enums.h"

G_BEGIN_DECLS

/*
 * Since the pulse animation surrounds the tip and would
 * exceed the canvas size, we need to scale it to fit the pulse.
 */
#define XRD_TIP_VIEWPORT_SCALE 3

/*
 * The distance in meters for which apparent size and regular size are equal.
 */
#define XRD_TIP_APPARENT_SIZE_DISTANCE 3.0f


#define XRD_TYPE_CLIENT xrd_client_get_type()
G_DECLARE_DERIVABLE_TYPE (XrdClient, xrd_client, XRD, CLIENT, GObject)

/**
 * XrdClientClass:
 * @parent: The object class structure needs to be the first
 *   element in the widget class structure in order for the class mechanism
 *   to work correctly. This allows a XrdClientClass pointer to be cast to
 *   a GObjectClass pointer.
 * @add_button: Create a label button.
 * @get_gulkan: Get a #GulkanClient from the #XrdClient.
 * @init_controller: Initialize a #XrdController.
 * @window_new_from_meters: Initialize a #XrdWindow.
 * @window_new_from_data: Initialize a #XrdWindow.
 * @window_new_from_pixels: Initialize a #XrdWindow.
 * @window_new_from_native: Initialize a #XrdWindow.
 */
struct _XrdClientClass
{
  GObjectClass parent;

  gboolean
  (*add_button) (XrdClient          *self,
                 XrdWindow         **button,
                 int                 label_count,
                 gchar             **label,
                 graphene_point3d_t *position,
                 GCallback           press_callback,
                 gpointer            press_callback_data);

  GulkanClient *
  (*get_gulkan) (XrdClient *self);

  void
  (*init_controller) (XrdClient     *self,
                      GxrController *controller);

  XrdWindow *
  (*window_new_from_meters) (XrdClient  *self,
                             const char *title,
                             float       width,
                             float       height,
                             float       ppm);

  XrdWindow *
  (*window_new_from_data) (XrdClient     *self,
                           XrdWindowData *data);

  XrdWindow *
  (*window_new_from_pixels) (XrdClient  *self,
                             const char *title,
                             uint32_t    width,
                             uint32_t    height,
                             float       ppm);

  XrdWindow *
  (*window_new_from_native) (XrdClient   *self,
                             const gchar *title,
                             gpointer     native,
                             uint32_t     width_pixels,
                             uint32_t     height_pixels,
                             float        ppm);
};

XrdClient *
xrd_client_new (void);

XrdClient *
xrd_client_new_with_mode (XrdClientMode mode);

void
xrd_client_add_container (XrdClient *self,
                          XrdContainer *container);

void
xrd_client_remove_container (XrdClient *self,
                             XrdContainer *container);

void
xrd_client_add_window (XrdClient *self,
                       XrdWindow *window,
                       gboolean   draggable,
                       gpointer   lookup_key);

XrdWindow *
xrd_client_lookup_window (XrdClient *self,
                          gpointer key);

void
xrd_client_remove_window (XrdClient *self,
                          XrdWindow *window);

XrdWindow*
xrd_client_button_new_from_text (XrdClient *self,
                                 float      width,
                                 float      height,
                                 float      ppm,
                                 int        label_count,
                                 gchar    **label);

XrdWindow*
xrd_client_button_new_from_icon (XrdClient   *self,
                                 float        width,
                                 float        height,
                                 float        ppm,
                                 const gchar *url);

void
xrd_client_add_button (XrdClient          *self,
                       XrdWindow          *button,
                       graphene_point3d_t *position,
                       GCallback           press_callback,
                       gpointer            press_callback_data);

XrdWindow *
xrd_client_get_keyboard_window (XrdClient *self);

GulkanClient *
xrd_client_get_gulkan (XrdClient *self);

XrdWindow *
xrd_client_get_synth_hovered (XrdClient *self);

XrdWindowManager *
xrd_client_get_manager (XrdClient *self);

void
xrd_client_set_pin (XrdClient *self,
                    XrdWindow *win,
                    gboolean pin);

void
xrd_client_show_pinned_only (XrdClient *self,
                             gboolean pinned_only);

XrdInputSynth *
xrd_client_get_input_synth (XrdClient *self);

gboolean
xrd_client_poll_runtime_events (XrdClient *self);

gboolean
xrd_client_poll_input_events (XrdClient *self);

XrdDesktopCursor*
xrd_client_get_desktop_cursor (XrdClient *self);

VkImageLayout
xrd_client_get_upload_layout (XrdClient *self);

GSList *
xrd_client_get_controllers (XrdClient *self);

GSList *
xrd_client_get_windows (XrdClient *self);

struct _XrdClient *
xrd_client_switch_mode (XrdClient *self);

GxrContext *
xrd_client_get_gxr_context (XrdClient *self);

G_END_DECLS

#endif /* XRD_CLIENT_H_ */
