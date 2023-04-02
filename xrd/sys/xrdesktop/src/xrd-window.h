/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_WINDOW_H_
#define XRD_WINDOW_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include <graphene.h>
#include <gulkan.h>

#include "xrd-types.h"

G_BEGIN_DECLS

typedef struct _XrdClient XrdClient;

#define XRD_TYPE_WINDOW xrd_window_get_type()
G_DECLARE_INTERFACE (XrdWindow, xrd_window, XRD, WINDOW, GObject)

/**
 * XrdWindowData:
 * @native: A native pointer to a window struct from a window manager.
 * @texture_width: The width of the last submitted texture in pixels.
 * @texture_height: The height of the last submitted texture in pixels.
 * @title: A window title.
 * @initial_size_meters: The window dimensions in meters without scale.
 * @scale: A user applied scale.
 * @transform: The transformation #graphene_matrix_t of the window.
 * @child_window: A window that is pinned on top of this window and follows this window's position and scaling.
 * @parent_window: The parent window, %NULL if the window does not have a parent.
 * @child_offset_center: If the window is a child, this stores the 2D offset to the parent in meters.
 * @reset_transform: The transformation that the window will be reset to.
 * @pinned: Whether the window will be visible in pinned only mode.
 * @texture: Cache of the currently rendered texture.
 * @xrd_window: A pointer to the #XrdWindow this XrdWindowData belongs to.
 * @owned_by_window: If TRUE, Xrd*Window finalization must free this XrdWindowData
 * After switching the overlay/scene mode, it will point to a new #XrdWindow.
 *
 * Common struct for scene and overlay windows.
 **/
typedef struct XrdWindowData {
  gpointer native;

  uint32_t texture_width;
  uint32_t texture_height;
  GString *title;

  graphene_point_t initial_size_meters;

  float scale;
  graphene_matrix_t transform;

  struct XrdWindowData *child_window;
  struct XrdWindowData *parent_window;

  graphene_point_t child_offset_center;

  graphene_matrix_t reset_transform;

  gboolean pinned;

  GulkanTexture *texture;

  XrdWindow *xrd_window;

  gboolean owned_by_window;
} XrdWindowData;

/**
 * XrdWindowInterface:
 * @parent: parent
 * @set_transformation: Set a #graphene_matrix_t transformation.
 * @get_transformation: Get a #graphene_matrix_t transformation including scale.
 * @get_transformation_no_scale: Get a #graphene_matrix_t transformation without scale.
 * @submit_texture: Submits current texture to the rendering backend.
 * @set_and_submit_texture: Sets and submits a new texture to the window.
 * @get_texture: Returns current window texture.
 * @poll_event: Poll events on the window.
 * @emit_grab_start: Emit an event when the grab action was started.
 * @emit_grab: Emit a continous event during the grab action.
 * @emit_release: Emit an event when the grab action was finished.
 * @emit_hover_end: Emit an event when hovering the window was finished.
 * @emit_hover: Emit a continous event when the window is hovered.
 * @emit_hover_start: Emit an event when hovering the window started.
 * @add_child: Add a child window.
 * @set_color: Set a color that is multiplied to the texture.
 * @set_flip_y: Flip the y axis of the texture.
 * @show: Show the window.
 * @hide: Hide the window.
 * @is_visible: Check if the window is currently visible.
 * @constructed: Common constructor.
 * @get_data: Get common data struct.
 * @windows_created: Static counter how many windows were created, used for creating automatic overlay names.
 **/
struct _XrdWindowInterface
{
  GTypeInterface parent;

  gboolean
  (*set_transformation) (XrdWindow         *self,
                         graphene_matrix_t *mat);

  gboolean
  (*get_transformation) (XrdWindow         *self,
                         graphene_matrix_t *mat);

  gboolean
  (*get_transformation_no_scale) (XrdWindow         *self,
                                  graphene_matrix_t *mat);

  void
  (*submit_texture) (XrdWindow *self);

  void
  (*set_and_submit_texture) (XrdWindow     *self,
                             GulkanTexture *texture);

  GulkanTexture *
  (*get_texture) (XrdWindow *self);

  void
  (*poll_event) (XrdWindow *self);

  void
  (*emit_grab_start) (XrdWindow *self, GxrController *controller);

  void
  (*emit_grab) (XrdWindow *self, XrdGrabEvent *event);

  void
  (*emit_release) (XrdWindow *self, GxrController *controller);

  void
  (*emit_hover_end) (XrdWindow *self, GxrController *controller);

  void
  (*emit_hover) (XrdWindow *self, XrdHoverEvent *event);

  void
  (*emit_hover_start) (XrdWindow *self, GxrController *controller);

  void
  (*add_child) (XrdWindow *self, XrdWindow *child,
                graphene_point_t *offset_center);

  void
  (*set_color) (XrdWindow *self, const graphene_vec3_t *color);

  void
  (*set_flip_y) (XrdWindow *self,
                 gboolean flip_y);

  void
  (*show) (XrdWindow *self);

  void
  (*hide) (XrdWindow *self);

  gboolean
  (*is_visible) (XrdWindow *self);

  void
  (*constructed) (GObject *object);

  XrdWindowData*
  (*get_data) (XrdWindow *self);

  guint windows_created;
};

XrdWindow *
xrd_window_new_from_data (XrdClient     *self,
                          XrdWindowData *data);

XrdWindow *
xrd_window_new_from_meters (XrdClient  *client,
                            const char *title,
                            float       width,
                            float       height,
                            float       ppm);

XrdWindow *
xrd_window_new_from_pixels (XrdClient  *client,
                            const char *title,
                            uint32_t    width,
                            uint32_t    height,
                            float       ppm);

XrdWindow *
xrd_window_new_from_native (XrdClient   *client,
                            const gchar *title,
                            gpointer     native,
                            uint32_t     width_pixels,
                            uint32_t     height_pixels,
                            float        ppm);

gboolean
xrd_window_set_transformation (XrdWindow *self, graphene_matrix_t *mat);

gboolean
xrd_window_get_transformation (XrdWindow *self, graphene_matrix_t *mat);

gboolean
xrd_window_get_transformation_no_scale (XrdWindow         *self,
                                        graphene_matrix_t *mat);

void
xrd_window_submit_texture (XrdWindow *self);

void
xrd_window_set_and_submit_texture (XrdWindow *self,
                                   GulkanTexture *texture);

GulkanTexture *
xrd_window_get_texture (XrdWindow *self);

void
xrd_window_poll_event (XrdWindow *self);

void
xrd_window_get_intersection_2d_pixels (XrdWindow          *self,
                                       graphene_point3d_t *intersection_3d,
                                       graphene_point_t   *intersection_pixels);

void
xrd_window_get_intersection_2d (XrdWindow          *self,
                                graphene_point3d_t *intersection_3d,
                                graphene_point_t   *intersection_2d);


void
xrd_window_emit_grab_start (XrdWindow *self,
                            GxrController *controller);

void
xrd_window_emit_grab (XrdWindow *self,
                      XrdGrabEvent *event);

void
xrd_window_emit_release (XrdWindow *self,
                         GxrController *controller);

void
xrd_window_emit_hover_end (XrdWindow *self,
                           GxrController *controller);

void
xrd_window_emit_hover (XrdWindow    *self,
                       XrdHoverEvent *event);

void
xrd_window_emit_hover_start (XrdWindow *self,
                             GxrController *controller);

void
xrd_window_add_child (XrdWindow *self,
                      XrdWindow *child,
                      graphene_point_t *offset_center);

void
xrd_window_select (XrdWindow *self);

void
xrd_window_deselect (XrdWindow *self);

gboolean
xrd_window_is_selected (XrdWindow *self);

void
xrd_window_end_selection (XrdWindow *self);

void
xrd_window_set_flip_y (XrdWindow *self,
                       gboolean flip_y);

float
xrd_window_get_current_ppm (XrdWindow *self);

float
xrd_window_get_initial_ppm (XrdWindow *self);

void
xrd_window_show (XrdWindow *self);

void
xrd_window_hide (XrdWindow *self);

gboolean
xrd_window_is_visible (XrdWindow *self);

float
xrd_window_get_current_width_meters (XrdWindow *self);

float
xrd_window_get_current_height_meters (XrdWindow *self);

XrdWindowData*
xrd_window_get_data (XrdWindow *self);

void
xrd_window_update_child (XrdWindow *self);

void
xrd_window_get_normal (XrdWindow       *self,
                       graphene_vec3_t *normal);

void
xrd_window_get_plane (XrdWindow        *self,
                      graphene_plane_t *res);

float
xrd_window_get_aspect_ratio (XrdWindow *self);

void
xrd_window_set_color (XrdWindow *self, const graphene_vec3_t *color);

void
xrd_window_save_reset_transformation (XrdWindow *self);

void
xrd_window_set_reset_transformation (XrdWindow *self,
                                     graphene_matrix_t *transform);

void
xrd_window_get_reset_transformation (XrdWindow *self,
                                     graphene_matrix_t *transform);

void
xrd_window_set_pin (XrdWindow *self,
                    gboolean pinned,
                    gboolean hide_unpinned);

gboolean
xrd_window_is_pinned (XrdWindow *self);

void
xrd_window_close (XrdWindow *self);

G_END_DECLS

#endif /* XRD_WINDOW_H_ */
