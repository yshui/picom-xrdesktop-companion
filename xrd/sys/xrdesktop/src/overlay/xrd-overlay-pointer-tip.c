/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>

#include <gxr.h>

#include "xrd-overlay-pointer-tip.h"
#include "xrd-settings.h"
#include "xrd-math.h"
#include "graphene-ext.h"

struct _XrdOverlayPointerTip
{
  GObject parent;

  GxrOverlay *overlay;
  GulkanTexture *texture;

  GxrPointerTipData data;
};

static void
xrd_overlay_pointer_tip_interface_init (GxrPointerTipInterface *iface);

G_DEFINE_TYPE_WITH_CODE (XrdOverlayPointerTip, xrd_overlay_pointer_tip,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_POINTER_TIP,
                                                xrd_overlay_pointer_tip_interface_init))

static void
xrd_overlay_pointer_tip_finalize (GObject *gobject);

static void
xrd_overlay_pointer_tip_class_init (XrdOverlayPointerTipClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_overlay_pointer_tip_finalize;
}

static void
_set_width_meters (GxrPointerTip *tip, float meters)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  gxr_overlay_set_width_meters (self->overlay, meters);
}

static void
xrd_overlay_pointer_tip_init (XrdOverlayPointerTip *self)
{
  self->data.active = FALSE;
  self->data.animation = NULL;
  self->data.upload_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

XrdOverlayPointerTip *
xrd_overlay_pointer_tip_new (GxrContext    *context,
                             GxrController *controller)
{
  XrdOverlayPointerTip *self =
    (XrdOverlayPointerTip*) g_object_new (XRD_TYPE_OVERLAY_POINTER_TIP, 0);

  char key[16];
  snprintf (key, 16 - 1, "tip-%ld",
            gxr_device_get_handle (GXR_DEVICE (controller)));
  self->overlay = gxr_overlay_new (context, key);
  if (!self->overlay)
    {
      g_printerr ("Intersection overlay unavailable.\n");
      return NULL;
    }

  /*
   * The tip should always be visible, except the pointer can
   * occlude it. The pointer has max sort order, so the tip gets max -1
   */
  gxr_overlay_set_sort_order (self->overlay, UINT32_MAX - 1);

  gxr_pointer_tip_init_settings (GXR_POINTER_TIP (self), &self->data);

  return self;
}

static void
xrd_overlay_pointer_tip_finalize (GObject *gobject)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (gobject);

  /* release the ref we set in pointer tip init */
  g_object_unref (self->overlay);

  if (self->texture)
    g_object_unref (self->texture);

  G_OBJECT_CLASS (xrd_overlay_pointer_tip_parent_class)->finalize (gobject);
}

static void
_set_transformation (GxrPointerTip     *tip,
                     graphene_matrix_t *matrix)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  gxr_overlay_set_transform_absolute (self->overlay, matrix);
}

static void
_get_transformation (GxrPointerTip     *tip,
                     graphene_matrix_t *matrix)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  gxr_overlay_get_transform_absolute (self->overlay, matrix);
}

static void
_show (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  gxr_overlay_show (self->overlay);
}

static void
_hide (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  gxr_overlay_hide (self->overlay);
}

static gboolean
_is_visible (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  return gxr_overlay_is_visible (self->overlay);
}

static GxrPointerTipData*
_get_data (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  return &self->data;
}

static GulkanClient*
_get_gulkan_client (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  GxrContext *context = gxr_overlay_get_context (self->overlay);
  return gxr_context_get_gulkan (context);
}

static GulkanTexture *
_get_texture (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  return self->texture;
}

static void
_submit_texture (GxrPointerTip *tip)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);

  GulkanTexture *texture = _get_texture (tip);
  if (!gxr_overlay_submit_texture (self->overlay, texture))
    g_warning ("Could not submit overlay pointer tip texture.\n");
}

static void
_set_and_submit_texture (GxrPointerTip *tip,
                         GulkanTexture *texture)
{
  XrdOverlayPointerTip *self = XRD_OVERLAY_POINTER_TIP (tip);
  if (self->texture == texture)
    {
      return;
    }
  else
    {
      GulkanTexture *to_free = self->texture;

      self->texture = texture;
      _submit_texture (tip);

      if (to_free)
        g_object_unref (to_free);
    }
}
static void
xrd_overlay_pointer_tip_interface_init (GxrPointerTipInterface *iface)
{
  iface->set_transformation = _set_transformation;
  iface->get_transformation = _get_transformation;
  iface->show = _show;
  iface->hide = _hide;
  iface->is_visible = _is_visible;
  iface->set_width_meters = _set_width_meters;
  iface->submit_texture = _submit_texture;
  iface->get_texture = _get_texture;
  iface->set_and_submit_texture = _set_and_submit_texture;
  iface->get_data = _get_data;
  iface->get_gulkan_client = _get_gulkan_client;
}

