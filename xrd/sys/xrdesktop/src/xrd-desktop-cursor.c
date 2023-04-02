/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-desktop-cursor.h"

#include <gxr.h>

#include "xrd-settings.h"
#include "graphene-ext.h"

#include "xrd-client.h"

G_DEFINE_INTERFACE (XrdDesktopCursor, xrd_desktop_cursor, G_TYPE_OBJECT)

static void
xrd_desktop_cursor_default_init (XrdDesktopCursorInterface *iface)
{
  (void) iface;
}

/**
 * xrd_desktop_cursor_submit_texture:
 * @self: The #XrdDesktopCursor
 *
 * Submits current texture.
 */
void
xrd_desktop_cursor_submit_texture (XrdDesktopCursor *self)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->submit_texture (self);
}

/**
 * xrd_desktop_cursor_set_and_submit_texture:
 * @self: The #XrdDesktopCursor
 * @texture: A #GulkanTexture that this #XrdDesktopCursor will own.
 *
 * Sets and submits a texture.
 */
void
xrd_desktop_cursor_set_and_submit_texture (XrdDesktopCursor *self,
                                           GulkanTexture    *texture)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->set_and_submit_texture (self, texture);
}

/**
 * xrd_desktop_cursor_set_hotspot:
 * @self: The #XrdDesktopCursor
 * @hotspot_x: The x component of the hotspot.
 * @hotspot_y: The y component of the hotspot.
 *
 * A hotspot of (x, y) means that the hotspot is at x pixels right, y pixels
 * down from the top left corner of the texture.
 */
void
xrd_desktop_cursor_set_hotspot (XrdDesktopCursor *self,
                                int               hotspot_x,
                                int               hotspot_y)
{
  XrdDesktopCursorData *data = xrd_desktop_cursor_get_data (self);
  data->hotspot_x = hotspot_x;
  data->hotspot_y = hotspot_y;
}

GulkanTexture *
xrd_desktop_cursor_get_texture (XrdDesktopCursor *self)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  return iface->get_texture (self);
}

void
xrd_desktop_cursor_show (XrdDesktopCursor *self)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->show (self);
}

void
xrd_desktop_cursor_hide (XrdDesktopCursor *self)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->hide (self);
}

void
xrd_desktop_cursor_set_width_meters (XrdDesktopCursor *self, float meters)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->set_width_meters (self, meters);
}

XrdDesktopCursorData*
xrd_desktop_cursor_get_data (XrdDesktopCursor *self)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  return iface->get_data (self);
}

void
xrd_desktop_cursor_get_transformation (XrdDesktopCursor  *self,
                                       graphene_matrix_t *matrix)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->get_transformation (self, matrix);
}

void
xrd_desktop_cursor_set_transformation (XrdDesktopCursor  *self,
                                       graphene_matrix_t *matrix)
{
  XrdDesktopCursorInterface* iface = XRD_DESKTOP_CURSOR_GET_IFACE (self);
  iface->set_transformation (self, matrix);
}

static void
_update_width_meters (GSettings *settings, gchar *key, gpointer _data)
{
  XrdDesktopCursorData *data = (XrdDesktopCursorData*) _data;
  XrdDesktopCursor *self = data->cursor;

  data->width_meters = (float) g_settings_get_double (settings, key);
  xrd_desktop_cursor_set_width_meters (self, data->width_meters);
}

static void
_update_apparent_size (XrdDesktopCursor   *self,
                       GxrContext         *context,
                       graphene_point3d_t *cursor_point)
{
  XrdDesktopCursorData *data = xrd_desktop_cursor_get_data (self);

  if (!data->keep_apparent_size)
    return;

  graphene_matrix_t hmd_pose;
  gboolean has_pose = gxr_context_get_head_pose (context, &hmd_pose);
  if (!has_pose)
    {
      xrd_desktop_cursor_set_width_meters (XRD_DESKTOP_CURSOR (self),
                                           data->width_meters);
      return;
    }

  graphene_point3d_t hmd_point;
  graphene_ext_matrix_get_translation_point3d (&hmd_pose, &hmd_point);

  float distance = graphene_point3d_distance (cursor_point, &hmd_point, NULL);

  /* divide distance by 3 so the width and the apparent width are the same at
   * a distance of 3 meters. This makes e.g. self->width = 0.3 look decent in
   * both cases at typical usage distances. */
  data->cached_width_meters = data->width_meters
                              / XRD_TIP_APPARENT_SIZE_DISTANCE
                              * distance;

  xrd_desktop_cursor_set_width_meters (XRD_DESKTOP_CURSOR (self),
                                       data->cached_width_meters);
}

static void
_update_keep_apparent_size (GSettings *settings, gchar *key, gpointer _data)
{
  XrdDesktopCursorData *data = (XrdDesktopCursorData*) _data;

  XrdDesktopCursor *self = data->cursor;
  data->keep_apparent_size = g_settings_get_boolean (settings, key);
  if (data->keep_apparent_size)
    {
      graphene_matrix_t cursor_pose;
      xrd_desktop_cursor_get_transformation (self, &cursor_pose);

      graphene_vec3_t cursor_point_vec;
      graphene_ext_matrix_get_translation_vec3 (&cursor_pose,
                                                &cursor_point_vec);
      graphene_point3d_t cursor_point;
      graphene_point3d_init_from_vec3 (&cursor_point, &cursor_point_vec);
    }

  xrd_desktop_cursor_set_width_meters (XRD_DESKTOP_CURSOR (self),
                                       data->width_meters);
}

void
xrd_desktop_cursor_init_settings (XrdDesktopCursor *self)
{
  XrdDesktopCursorData *data = xrd_desktop_cursor_get_data (self);
  data->cursor = self;

  xrd_settings_connect_and_apply (G_CALLBACK (_update_width_meters),
                                  "desktop-cursor-width-meters", data);

  xrd_settings_connect_and_apply (G_CALLBACK (_update_keep_apparent_size),
                                  "pointer-tip-keep-apparent-size", data);
}

void
xrd_desktop_cursor_update (XrdDesktopCursor   *self,
                           GxrContext         *context,
                           XrdWindow          *window,
                           graphene_point3d_t *intersection)
{
  XrdDesktopCursorData *data = xrd_desktop_cursor_get_data (self);

  if (data->texture_width == 0 || data->texture_height == 0)
    return;

  /* size needs to be set first, so the hotspot offset calculation works */
  _update_apparent_size (XRD_DESKTOP_CURSOR (self), context, intersection);

  /* Calculate the position of the cursor in the space of the window it is "on",
   * because the cursor is rotated in 3d to lie on the overlay's plane:
   * Move a point (center of the cursor) from the origin
   * 1) To the offset it has on the overlay it is on.
   *    This places the cursor's center at the target point on the overlay.
   * 2) half the width of the cursor right, half the height down.
   *    This places the upper left corner of the cursor at the target point.
   * 3) the offset of the hotspot up/left.
   *    This places exactly the hotspot at the target point. */

  graphene_point_t intersection_2d;
  xrd_window_get_intersection_2d (window, intersection, &intersection_2d);

  graphene_point3d_t offset_3d;
  graphene_point3d_init (&offset_3d, intersection_2d.x, intersection_2d.y, 0);

  graphene_matrix_t transform;
  graphene_matrix_init_translate (&transform, &offset_3d);

  /* TODO: following assumes width==height. Are there non quadratic cursors? */

  graphene_point3d_t cursor_radius;
  graphene_point3d_init (&cursor_radius,
                         data->cached_width_meters / 2.f,
                         - data->cached_width_meters / 2.f, 0);
  graphene_matrix_translate (&transform, &cursor_radius);

  float cursor_hotspot_meter_x = - (float) data->hotspot_x /
      ((float)data->texture_width) * data->cached_width_meters;
  float cursor_hotspot_meter_y = + (float) data->hotspot_y /
      ((float)data->texture_height) * data->cached_width_meters;

  graphene_point3d_t cursor_hotspot;
  graphene_point3d_init (&cursor_hotspot,
                         cursor_hotspot_meter_x,
                         cursor_hotspot_meter_y, 0);
  graphene_matrix_translate (&transform, &cursor_hotspot);

  graphene_matrix_t overlay_transform;
  xrd_window_get_transformation_no_scale (window, &overlay_transform);
  graphene_matrix_multiply (&transform, &overlay_transform, &transform);

  xrd_desktop_cursor_set_transformation (self, &transform);
}
