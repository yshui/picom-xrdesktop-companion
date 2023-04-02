/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-overlay-desktop-cursor.h"

#include <gxr.h>

#include "xrd-settings.h"
#include "xrd-math.h"
#include "graphene-ext.h"
#include "xrd-desktop-cursor.h"

struct _XrdOverlayDesktopCursor
{
  GObject parent;

  GxrOverlay *overlay;

  GulkanTexture *texture;

  XrdDesktopCursorData data;
};

static void
xrd_overlay_desktop_cursor_interface_init (XrdDesktopCursorInterface *iface);

G_DEFINE_TYPE_WITH_CODE (XrdOverlayDesktopCursor, xrd_overlay_desktop_cursor, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XRD_TYPE_DESKTOP_CURSOR,
                                                xrd_overlay_desktop_cursor_interface_init))

static void
xrd_overlay_desktop_cursor_finalize (GObject *gobject);

static void
xrd_overlay_desktop_cursor_class_init (XrdOverlayDesktopCursorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_overlay_desktop_cursor_finalize;
}

static void
_init_overlay (XrdOverlayDesktopCursor *self, GxrContext *context)
{
  self->overlay = gxr_overlay_new (context, "org.xrdesktop.cursor");
  if (!self->overlay)
    {
      g_printerr ("Cursor overlay unavailable.\n");
      return;
    }

  xrd_desktop_cursor_init_settings (XRD_DESKTOP_CURSOR (self));

  /* pointer ray is MAX, pointer tip is MAX - 1, so cursor is MAX - 2 */
  gxr_overlay_set_sort_order (self->overlay, UINT32_MAX - 2);
  gxr_overlay_show (self->overlay);
}

static void
xrd_overlay_desktop_cursor_init (XrdOverlayDesktopCursor *self)
{
  self->data.texture_width = 0;
  self->data.texture_height = 0;
  self->texture = NULL;
}

XrdOverlayDesktopCursor *
xrd_overlay_desktop_cursor_new (GxrContext *context)
{
  XrdOverlayDesktopCursor *self =
    (XrdOverlayDesktopCursor*) g_object_new (XRD_TYPE_OVERLAY_DESKTOP_CURSOR, 0);

  _init_overlay (self, context);
  return self;
}

static GulkanTexture *
_get_texture (XrdDesktopCursor *cursor)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  return self->texture;
}

static void
_submit_texture (XrdDesktopCursor *cursor)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);

  GulkanTexture *texture = _get_texture (cursor);
  gxr_overlay_submit_texture (self->overlay, texture);
}

static void
_set_and_submit_texture (XrdDesktopCursor *cursor,
                         GulkanTexture    *texture)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);

  gxr_overlay_submit_texture (self->overlay, texture);

  GulkanTexture *to_free = self->texture;

  self->texture = texture;
  _submit_texture (cursor);

  VkExtent2D extent = gulkan_texture_get_extent (texture);

  self->data.texture_width = extent.width;
  self->data.texture_height = extent.height;

  if (to_free)
    g_object_unref (to_free);
}

static void
_show (XrdDesktopCursor *cursor)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  gxr_overlay_show (self->overlay);
}

static void
_hide (XrdDesktopCursor *cursor)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  gxr_overlay_hide (self->overlay);
}

static void
xrd_overlay_desktop_cursor_finalize (GObject *gobject)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (gobject);

  if (self->texture)
    g_clear_object (&self->texture);

  g_object_unref (self->overlay);

  G_OBJECT_CLASS (xrd_overlay_desktop_cursor_parent_class)->finalize (gobject);
}

static XrdDesktopCursorData*
_get_data (XrdDesktopCursor *cursor)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  return &self->data;
}

static void
_get_transformation (XrdDesktopCursor  *cursor,
                     graphene_matrix_t *matrix)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  gxr_overlay_get_transform_absolute (self->overlay, matrix);
}

static void
_set_transformation (XrdDesktopCursor  *cursor,
                     graphene_matrix_t *matrix)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  gxr_overlay_set_transform_absolute (self->overlay, matrix);
}

static void
_set_width_meters (XrdDesktopCursor *cursor,
                   float             width)
{
  XrdOverlayDesktopCursor *self = XRD_OVERLAY_DESKTOP_CURSOR (cursor);
  if (!gxr_overlay_set_width_meters (self->overlay, width))
    g_warning ("Could not set overlay desktop cursor width to %f\n",
               (double) width);
}

static void
xrd_overlay_desktop_cursor_interface_init (XrdDesktopCursorInterface *iface)
{
  iface->submit_texture = _submit_texture;
  iface->get_texture = _get_texture;
  iface->set_and_submit_texture = _set_and_submit_texture;
  iface->show = _show;
  iface->hide = _hide;
  iface->set_width_meters = _set_width_meters;
  iface->get_data = _get_data;
  iface->get_transformation = _get_transformation;
  iface->set_transformation = _set_transformation;
}
