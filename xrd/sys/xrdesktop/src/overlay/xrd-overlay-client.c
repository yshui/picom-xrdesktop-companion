/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-overlay-client-private.h"

#include "xrd-client-private.h"
#include "xrd-overlay-window-private.h"

#include "xrd-overlay-desktop-cursor.h"
#include "xrd-overlay-pointer.h"
#include "xrd-overlay-pointer-tip.h"

struct _XrdOverlayClient
{
  XrdClient parent;

  gboolean pinned_only;
  XrdOverlayWindow *pinned_button;
};

G_DEFINE_TYPE (XrdOverlayClient, xrd_overlay_client, XRD_TYPE_CLIENT)

static void
xrd_overlay_client_finalize (GObject *gobject);

static void
xrd_overlay_client_init (XrdOverlayClient *self)
{
  xrd_client_set_upload_layout (XRD_CLIENT (self),
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  self->pinned_only = FALSE;
}

XrdOverlayClient *
xrd_overlay_client_new (GxrContext *context)
{
  XrdOverlayClient *self =
    (XrdOverlayClient*) g_object_new (XRD_TYPE_OVERLAY_CLIENT, 0);

  xrd_client_set_context (XRD_CLIENT (self), context);

  xrd_client_init_input_callbacks (XRD_CLIENT (self));

  XrdDesktopCursor *cursor =
    XRD_DESKTOP_CURSOR (xrd_overlay_desktop_cursor_new (context));
  xrd_client_set_desktop_cursor (XRD_CLIENT (self), cursor);

  return self;
}

static void
xrd_overlay_client_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (xrd_overlay_client_parent_class)->finalize (gobject);
}

static GulkanClient *
_get_gulkan (XrdClient *client)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  return gxr_context_get_gulkan (context);
}

static void
_init_controller (XrdClient     *client,
                  GxrController *controller)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  GxrPointer *pointer_ray = GXR_POINTER (xrd_overlay_pointer_new ());
  if (pointer_ray == NULL)
    {
      g_printerr ("Error: Could not init pointer %lu\n",
                  gxr_device_get_handle (GXR_DEVICE (controller)));
      return;
    }
  gxr_controller_set_pointer (controller, pointer_ray);

  GxrPointerTip *pointer_tip =
    GXR_POINTER_TIP (xrd_overlay_pointer_tip_new (context, controller));
  if (pointer_tip == NULL)
    {
      g_printerr ("Error: Could not init pointer tip %lu\n",
                  gxr_device_get_handle (GXR_DEVICE (controller)));
      return;
    }

  gxr_pointer_tip_set_active (pointer_tip, FALSE);
  gxr_pointer_tip_show (pointer_tip);

  gxr_controller_set_pointer_tip (controller, pointer_tip);
}

static XrdWindow *
_window_new_from_meters (XrdClient  *client,
                         const char *title,
                         float       width,
                         float       height,
                         float       ppm)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  return XRD_WINDOW (xrd_overlay_window_new_from_meters (context, title, width,
                                                         height, ppm));
}

static XrdWindow *
_window_new_from_data (XrdClient *client, XrdWindowData *data)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  return XRD_WINDOW (xrd_overlay_window_new_from_data (context, data));
}

static XrdWindow *
_window_new_from_pixels (XrdClient  *client,
                         const char *title,
                         uint32_t    width,
                         uint32_t    height,
                         float       ppm)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  return XRD_WINDOW (xrd_overlay_window_new_from_pixels (context, title, width,
                                                         height, ppm));
}

static XrdWindow *
_window_new_from_native (XrdClient   *client,
                         const gchar *title,
                         gpointer     native,
                         uint32_t     width_pixels,
                         uint32_t     height_pixels,
                         float        ppm)
{
  GxrContext *context = xrd_client_get_gxr_context (client);
  return XRD_WINDOW (xrd_overlay_window_new_from_native (context, title, native,
                                                         width_pixels,
                                                         height_pixels,
                                                         ppm));
}

static void
xrd_overlay_client_class_init (XrdOverlayClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_overlay_client_finalize;

  XrdClientClass *xrd_client_class = XRD_CLIENT_CLASS (klass);
  xrd_client_class->get_gulkan = _get_gulkan;
  xrd_client_class->init_controller = _init_controller;

  xrd_client_class->window_new_from_meters = _window_new_from_meters;
  xrd_client_class->window_new_from_data = _window_new_from_data;
  xrd_client_class->window_new_from_pixels = _window_new_from_pixels;
  xrd_client_class->window_new_from_native = _window_new_from_native;
}
