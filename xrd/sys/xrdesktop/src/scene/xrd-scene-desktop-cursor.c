/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-desktop-cursor.h"

#include "xrd-desktop-cursor.h"
#include "xrd-scene-window-private.h"

struct _XrdSceneDesktopCursor
{
  XrdSceneWindow parent;

  XrdDesktopCursorData data;
};

static void
xrd_scene_desktop_cursor_interface_init (XrdDesktopCursorInterface *iface);

G_DEFINE_TYPE_WITH_CODE (XrdSceneDesktopCursor, xrd_scene_desktop_cursor,
                         XRD_TYPE_SCENE_WINDOW,
                         G_IMPLEMENT_INTERFACE (XRD_TYPE_DESKTOP_CURSOR,
                                                xrd_scene_desktop_cursor_interface_init))

static void
xrd_scene_desktop_cursor_finalize (GObject *gobject);

static void
xrd_scene_desktop_cursor_class_init (XrdSceneDesktopCursorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_desktop_cursor_finalize;
}

static void
xrd_scene_desktop_cursor_init (XrdSceneDesktopCursor *self)
{
  self->data.texture_width = 0;
  self->data.texture_height = 0;
}

XrdSceneDesktopCursor *
xrd_scene_desktop_cursor_new (GulkanClient          *gulkan,
                              VkDescriptorSetLayout *layout,
                              VkBuffer               lights)
{
  XrdSceneDesktopCursor *self =
    (XrdSceneDesktopCursor*) g_object_new (XRD_TYPE_SCENE_DESKTOP_CURSOR, 0);

  g_object_set (self,
                "texture-width", 64,
                "texture-height", 64,
                NULL);

  xrd_scene_window_initialize (XRD_SCENE_WINDOW (self), gulkan, layout, lights);

  xrd_desktop_cursor_init_settings (XRD_DESKTOP_CURSOR (self));

  return self;
}

static void
xrd_scene_desktop_cursor_finalize (GObject *gobject)
{
  XrdSceneDesktopCursor *self = XRD_SCENE_DESKTOP_CURSOR (gobject);
  (void) self;

  G_OBJECT_CLASS (xrd_scene_desktop_cursor_parent_class)->finalize (gobject);
}

static GulkanTexture *
_get_texture (XrdDesktopCursor *cursor)
{
  XrdSceneDesktopCursor *self = XRD_SCENE_DESKTOP_CURSOR (cursor);
  XrdWindow *window = XRD_WINDOW (self);
  XrdWindowData *data = xrd_window_get_data (window);
  return data->texture;
}

static void
_submit_texture (XrdDesktopCursor *cursor)
{
  XrdSceneDesktopCursor *self = XRD_SCENE_DESKTOP_CURSOR (cursor);
  XrdSceneWindow *window = XRD_SCENE_WINDOW (self);
  xrd_window_submit_texture (XRD_WINDOW (window));
}

static void
_set_and_submit_texture (XrdDesktopCursor *cursor,
                         GulkanTexture    *texture)
{
  XrdSceneDesktopCursor *self = XRD_SCENE_DESKTOP_CURSOR (cursor);
  XrdWindow *window = XRD_WINDOW (self);

  GulkanTexture *cached_texture = _get_texture (cursor);
  if (cached_texture != texture)
    {
      VkExtent2D extent = gulkan_texture_get_extent (texture);
      self->data.texture_width = extent.width;
      self->data.texture_height = extent.height;
      xrd_window_set_and_submit_texture (XRD_WINDOW (window), texture);
    }
}

static XrdDesktopCursorData*
_get_data (XrdDesktopCursor *cursor)
{
  XrdSceneDesktopCursor *self = XRD_SCENE_DESKTOP_CURSOR (cursor);
  return &self->data;
}

static void
_get_transformation (XrdDesktopCursor  *cursor,
                     graphene_matrix_t *matrix)
{
  XrdSceneDesktopCursor *self = XRD_SCENE_DESKTOP_CURSOR (cursor);
  graphene_matrix_t transformation;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self),
                                       &transformation);
  graphene_matrix_init_from_matrix (matrix, &transformation);
}

static void
xrd_scene_desktop_cursor_interface_init (XrdDesktopCursorInterface *iface)
{
  iface->submit_texture = _submit_texture;
  iface->get_texture = _get_texture;
  iface->set_and_submit_texture = _set_and_submit_texture;
  iface->get_data = _get_data;
  iface->get_transformation = _get_transformation;
  iface->show = (void (*)(XrdDesktopCursor *)) xrd_scene_object_show;
  iface->hide = (void (*)(XrdDesktopCursor *)) xrd_scene_object_hide;
  iface->set_width_meters = (void (*)(XrdDesktopCursor *, float)) xrd_scene_window_set_width_meters;
  iface->set_transformation = (void (*)(XrdDesktopCursor *, graphene_matrix_t *)) xrd_scene_object_set_transformation;
}
