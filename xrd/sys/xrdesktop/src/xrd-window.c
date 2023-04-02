/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-window.h"
#include <gdk/gdk.h>

#include "graphene-ext.h"
#include "xrd-client-private.h"

#define SCALE_MIN_FACTOR .05f
#define SCALE_MAX_FACTOR 15.f

G_DEFINE_INTERFACE (XrdWindow, xrd_window, G_TYPE_OBJECT)

enum {
  MOTION_NOTIFY_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SHOW,
  DESTROY,
  SCROLL_EVENT,
  KEYBOARD_PRESS_EVENT,
  KEYBOARD_CLOSE_EVENT,
  GRAB_START_EVENT,
  GRAB_EVENT,
  RELEASE_EVENT,
  HOVER_START_EVENT,
  HOVER_EVENT,
  HOVER_END_EVENT,
  LAST_SIGNAL
};

static guint window_signals[LAST_SIGNAL] = { 0 };

static void
xrd_window_default_init (XrdWindowInterface *iface)
{
  window_signals[MOTION_NOTIFY_EVENT] =
    g_signal_new ("motion-notify-event",
                   G_TYPE_FROM_INTERFACE (iface),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
                   G_TYPE_FROM_INTERFACE (iface),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
                   G_TYPE_FROM_INTERFACE (iface),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[SHOW] =
    g_signal_new ("show",
                   G_TYPE_FROM_INTERFACE (iface),
                   G_SIGNAL_RUN_FIRST,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  window_signals[DESTROY] =
    g_signal_new ("destroy",
                   G_TYPE_FROM_INTERFACE (iface),
                     G_SIGNAL_RUN_CLEANUP |
                      G_SIGNAL_NO_RECURSE |
                      G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  window_signals[SCROLL_EVENT] =
    g_signal_new ("scroll-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[KEYBOARD_PRESS_EVENT] =
    g_signal_new ("keyboard-press-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[KEYBOARD_CLOSE_EVENT] =
    g_signal_new ("keyboard-close-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  window_signals[GRAB_START_EVENT] =
    g_signal_new ("grab-start-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[GRAB_EVENT] =
    g_signal_new ("grab-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  window_signals[RELEASE_EVENT] =
    g_signal_new ("release-event",
                  G_TYPE_FROM_INTERFACE (iface),
                   G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  window_signals[HOVER_END_EVENT] =
    g_signal_new ("hover-end-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  window_signals[HOVER_EVENT] =
    g_signal_new ("hover-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  window_signals[HOVER_START_EVENT] =
    g_signal_new ("hover-start-event",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  GParamSpec *pspec;
  pspec =
      g_param_spec_string ("title",
                           "Title",
                           "Title of the Window.",
                           "Untitled",
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  pspec =
    g_param_spec_float ("scale",
                       "Scaling Factor",
                       "Scaling Factor of this Window.",
                       SCALE_MIN_FACTOR,  /* minimum value */
                       SCALE_MAX_FACTOR, /* maximum value */
                       1.f,  /* default value */
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  pspec =
    g_param_spec_pointer ("native",
                          "Native Window Handle",
                          "A pointer to an (opaque) native window struct.",
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  pspec =
    g_param_spec_uint ("texture-width",
                       "Texture width",
                       "The width of the set texture.",
                       0,
                       32768,
                       0,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  pspec =
    g_param_spec_uint ("texture-height",
                       "Texture height",
                       "The height of the set texture.",
                       0,
                       32768,
                       0,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  pspec =
    g_param_spec_float ("initial-width-meters",
                       "Initial width (meters)",
                       "Initial window width in meters. "
                       "texture-width / initial-window-width determines the "
                       "ppm the window was originally created with. "
                       "The current width is initial-window-width * scale",
                       0.01f,
                       1000.0f,
                       1.0f,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  pspec =
    g_param_spec_float ("initial-height-meters",
                       "Initial height (meters)",
                       "Initial window height in meters."
                       "texture-height / initial-window-height determines the "
                       "ppm the window was originally created with. "
                       "The current height is initial-window-height * scale",
                       0.01f,
                       1000.0f,
                       1.0f,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_interface_install_property (iface, pspec);

  iface->windows_created = 0;
}

XrdWindow *
xrd_window_new_from_data (XrdClient     *self,
                          XrdWindowData *data)
{
  return xrd_client_window_new_from_data (self, data);
}

XrdWindow *
xrd_window_new_from_meters (XrdClient  *client,
                            const char *title,
                            float       width,
                            float       height,
                            float       ppm)
{
  return xrd_client_window_new_from_meters (client, title, width, height, ppm);
}

XrdWindow *
xrd_window_new_from_pixels (XrdClient  *client,
                            const char *title,
                            uint32_t    width,
                            uint32_t    height,
                            float       ppm)
{
  return xrd_client_window_new_from_pixels (client, title, width, height, ppm);
}

XrdWindow *
xrd_window_new_from_native (XrdClient   *client,
                            const gchar *title,
                            gpointer     native,
                            uint32_t     width_pixels,
                            uint32_t     height_pixels,
                            float        ppm)
{
  return xrd_client_window_new_from_native (client, title, native,
                                            width_pixels, height_pixels, ppm);
}

gboolean
xrd_window_set_transformation (XrdWindow *self, graphene_matrix_t *mat)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  return iface->set_transformation (self, mat);
}

gboolean
xrd_window_get_transformation (XrdWindow *self, graphene_matrix_t *mat)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  return iface->get_transformation (self, mat);
}

gboolean
xrd_window_get_transformation_no_scale (XrdWindow         *self,
                                        graphene_matrix_t *mat)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  return iface->get_transformation_no_scale (self, mat);
}

/**
 * xrd_window_submit_texture:
 * @self: The #XrdWindow
 * After new content has been rendered into a texture, this function must be
 * called to reflect the changed texture on screen.
 */
void
xrd_window_submit_texture (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->submit_texture (self);
}

/**
 * xrd_window_set_and_submit_texture:
 * @self: The #XrdWindow
 * @texture: A #GulkanTexture that is created by the caller.
 * Ownership of this texture is transferred to the #XrdWindow.
 *
 * For performance reason it is a good idea to not set a new texture every
 * time the window content changes. Instead the the cached texture should be
 * acquired with xrd_window_get_texture() and resubmitted with
 * xrd_window_submit_texture()..
 * If the window/texture size changes, a new texture should be submitted
 * (the previous submitted texture does not need to be unref'ed).
 *
 * This function also submits the @texture.
 */
void
xrd_window_set_and_submit_texture (XrdWindow     *self,
                                   GulkanTexture *texture)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->set_and_submit_texture (self, texture);
}

/**
 * xrd_window_get_texture:
 * @self: The #XrdWindow
 * Returns: The last #GulkanTexture submitted with xrd_window_submit_texture(),
 * or `NULL` if none has been submitted so far.
 */
GulkanTexture *
xrd_window_get_texture (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  return iface->get_texture (self);
}

float
xrd_window_get_current_ppm (XrdWindow *self)
{
  guint texture_width;
  gfloat width_meters;
  gfloat scale;

  g_object_get (self,
                "texture-width", &texture_width,
                "initial-width-meters", &width_meters,
                "scale", &scale,
                NULL);

  return (float) texture_width / (width_meters * scale);
}

float
xrd_window_get_initial_ppm (XrdWindow *self)
{
  guint texture_width;
  gfloat width_meters;

  g_object_get (self,
                "texture-width", &texture_width,
                "initial-width-meters", &width_meters,
                NULL);

  return (float) texture_width / width_meters;
}

/**
 * xrd_window_get_current_width_meters:
 * @self: The #XrdWindow
 *
 * Returns: The current world space width of the window in meters.
 */
float
xrd_window_get_current_width_meters (XrdWindow *self)
{
  float initial_width_meters;
  float scale;

  g_object_get (self,
                "scale", &scale,
                "initial-width-meters", &initial_width_meters,
                NULL);

  return initial_width_meters * scale;
}

/**
 * xrd_window_get_current_height_meters:
 * @self: The #XrdWindow
 *
 * Returns: The current world space height of the window in meters.
 */
float
xrd_window_get_current_height_meters (XrdWindow *self)
{
  float initial_height_meters;
  float scale;

  g_object_get (self,
                "scale", &scale,
                "initial-height-meters", &initial_height_meters,
                NULL);

  return initial_height_meters * scale;
}

/**
 * xrd_window_poll_event:
 * @self: The #XrdWindow
 *
 * Must be called periodically to receive events from this window.
 */
void
xrd_window_poll_event (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->poll_event (self);
}

/**
 * xrd_window_get_intersection_2d_pixels:
 * @self: The #XrdWindow
 * @intersection_3d: A #graphene_point3d_t intersection point in meters.
 * @intersection_pixels: Intersection in window coordinates with the origin at top/left in pixels.
 */

void
xrd_window_get_intersection_2d_pixels (XrdWindow          *self,
                                       graphene_point3d_t *intersection_3d,
                                       graphene_point_t   *intersection_pixels)
{
  /* transform intersection point to origin */
  graphene_point_t intersection_2d_point;
  xrd_window_get_intersection_2d (self, intersection_3d,
                                  &intersection_2d_point);

  graphene_vec2_t intersection_2d_vec;
  graphene_point_to_vec2 (&intersection_2d_point,
                          &intersection_2d_vec);


  /* normalize coordinates to [0 - 1, 0 - 1] */
  graphene_vec2_t size_meters;
  graphene_vec2_init (&size_meters,
                      xrd_window_get_current_width_meters (self),
                      xrd_window_get_current_height_meters (self));

  graphene_vec2_divide (&intersection_2d_vec, &size_meters,
                        &intersection_2d_vec);

  /* move origin from center to corner of overlay */
  graphene_vec2_t center_normalized;
  graphene_vec2_init (&center_normalized, 0.5f, 0.5f);

  graphene_vec2_add (&intersection_2d_vec, &center_normalized,
                     &intersection_2d_vec);

  /* invert y axis */
  graphene_vec2_init (&intersection_2d_vec,
                      graphene_vec2_get_x (&intersection_2d_vec),
                      1.0f - graphene_vec2_get_y (&intersection_2d_vec));

  /* scale to pixel coordinates */
  VkExtent2D size_pixels;
  g_object_get (self,
                "texture-width", &size_pixels.width,
                "texture-height", &size_pixels.height,
                NULL);

  graphene_vec2_t size_pixels_vec;
  graphene_vec2_init (&size_pixels_vec,
                      (float) size_pixels.width,
                      (float) size_pixels.height);

  graphene_vec2_multiply (&intersection_2d_vec, &size_pixels_vec,
                          &intersection_2d_vec);

  /* return point_t */
  graphene_point_init_from_vec2 (intersection_pixels, &intersection_2d_vec);
}

/**
 * xrd_window_get_intersection_2d:
 * @self: The #XrdWindow
 * @intersection_3d: A #graphene_point3d_t intersection point in meters.
 * @intersection_2d: Intersection in window coordinates with origin at center in meters.
 *
 * Calculates the offset of the intersection relative to the overlay's center,
 * in overlay-relative coordinates, in meters
 */
void
xrd_window_get_intersection_2d (XrdWindow          *self,
                                graphene_point3d_t *intersection_3d,
                                graphene_point_t   *intersection_2d)
{
  graphene_matrix_t transform;
  xrd_window_get_transformation_no_scale (self, &transform);

  graphene_matrix_t inverse_transform;
  graphene_matrix_inverse (&transform, &inverse_transform);

  graphene_point3d_t intersection_origin;
  graphene_matrix_transform_point3d (&inverse_transform,
                                      intersection_3d,
                                     &intersection_origin);

  graphene_point_init (intersection_2d,
                       intersection_origin.x,
                       intersection_origin.y);
}

void
xrd_window_emit_grab_start (XrdWindow *self, GxrController *controller)
{
  g_signal_emit (self, window_signals[GRAB_START_EVENT], 0, controller);
}


void
xrd_window_emit_grab (XrdWindow *self,
                      XrdGrabEvent *event)
{
  g_signal_emit (self, window_signals[GRAB_EVENT], 0, event);
}

void
xrd_window_emit_release (XrdWindow *self,GxrController *controller)
{
  g_signal_emit (self, window_signals[RELEASE_EVENT], 0, controller);
}

void
xrd_window_emit_hover_end (XrdWindow *self, GxrController *controller)
{
  g_signal_emit (self, window_signals[HOVER_END_EVENT], 0, controller);
}


void
xrd_window_emit_hover (XrdWindow    *self,
                       XrdHoverEvent *event)
{
  g_signal_emit (self, window_signals[HOVER_EVENT], 0, event);
}

void
xrd_window_emit_hover_start (XrdWindow *self, GxrController *controller)
{
  g_signal_emit (self, window_signals[HOVER_START_EVENT], 0, controller);
}

static void
_inherit_state_from_parent (XrdWindow *parent, XrdWindow *child)
{
  /* child window should inherit pinned status */
  xrd_window_set_pin (child, xrd_window_is_pinned (parent), FALSE);
  /* we don't know if client is in pinned only modus, but it is better to
   * inherit visibility from parent anyway. */
  if (xrd_window_is_visible (parent))
    xrd_window_show (child);
  else
    xrd_window_hide (child);
}

/**
 * xrd_window_add_child:
 * @self: The #XrdWindow
 * @child: An already existing window.
 * @offset_center: The offset of the child window's center to the parent
 * window's center in pixels.
 *
 * x axis points right, y axis points up.
 */
void
xrd_window_add_child (XrdWindow        *self,
                      XrdWindow        *child,
                      graphene_point_t *offset_center)
{
  if (!child)
    return;

  XrdWindowData *data = xrd_window_get_data (self);
  XrdWindowData *child_data = xrd_window_get_data (child);
  data->child_window = child_data;
  graphene_point_init_from_point (&data->child_offset_center, offset_center);

  xrd_window_update_child (self);

  child_data->parent_window = data;

  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->add_child (self, child, offset_center);

  _inherit_state_from_parent (self, child);
}

void
xrd_window_select (XrdWindow *self)
{
  graphene_vec3_t marked_color;
  graphene_vec3_init (&marked_color, 0.0f, 0.0f, 1.0f);
  xrd_window_set_color (self, &marked_color);
}

void
xrd_window_deselect (XrdWindow *self)
{
  graphene_vec3_t marked_color;
  graphene_vec3_init (&marked_color, 0.1f, 0.1f, 0.1f);
  xrd_window_set_color (self, &marked_color);
}

void
xrd_window_end_selection (XrdWindow *self)
{
  graphene_vec3_t unmarked_color;
  graphene_vec3_init (&unmarked_color, 1.f, 1.f, 1.f);
  xrd_window_set_color (self, &unmarked_color);
}

void
xrd_window_set_flip_y (XrdWindow *self,
                       gboolean flip_y)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->set_flip_y (self, flip_y);
}

void
xrd_window_show (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->show (self);
}

void
xrd_window_set_color (XrdWindow *self, const graphene_vec3_t *color)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->set_color (self, color);
}

void
xrd_window_hide (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  iface->hide (self);
}

gboolean
xrd_window_is_visible (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  return iface->is_visible (self);
}

XrdWindowData*
xrd_window_get_data (XrdWindow *self)
{
  XrdWindowInterface* iface = XRD_WINDOW_GET_IFACE (self);
  return iface->get_data (self);
}

void
xrd_window_update_child (XrdWindow *self)
{
  XrdWindowData *data = xrd_window_get_data (self);
  XrdWindow *child = data->child_window->xrd_window;

  /* this can happen during the overlay scene switch */
  if (!child)
    return;

  g_object_set (G_OBJECT(child), "scale", (double) data->scale, NULL);

  float initial_ppm = xrd_window_get_initial_ppm (XRD_WINDOW (self));

  graphene_point_t scaled_offset;
  graphene_ext_point_scale (&data->child_offset_center,
                             data->scale / initial_ppm,
                            &scaled_offset);

  graphene_point3d_t scaled_offset3d = {
    .x = scaled_offset.x,
    .y = scaled_offset.y,
    .z = 0.01f
  };
  graphene_matrix_t child_transform;
  graphene_matrix_init_translate (&child_transform, &scaled_offset3d);

  graphene_matrix_t parent_transform;
  xrd_window_get_transformation_no_scale (self, &parent_transform);

  graphene_matrix_multiply (&child_transform, &parent_transform,
                            &child_transform);

  xrd_window_set_transformation (XRD_WINDOW (child), &child_transform);
}

void
xrd_window_get_normal (XrdWindow       *self,
                       graphene_vec3_t *normal)
{
  graphene_vec3_init (normal, 0, 0, 1);

  graphene_matrix_t model_matrix;
  xrd_window_get_transformation (self, &model_matrix);

  graphene_matrix_t rotation_matrix;
  graphene_ext_matrix_get_rotation_matrix (&model_matrix,
                                           &rotation_matrix);

  graphene_matrix_transform_vec3 (&rotation_matrix, normal, normal);
}

void
xrd_window_get_plane (XrdWindow        *self,
                      graphene_plane_t *res)
{
  graphene_vec3_t normal;
  xrd_window_get_normal (self, &normal);

  graphene_matrix_t model_matrix;
  xrd_window_get_transformation (self, &model_matrix);

  graphene_point3d_t position;
  graphene_ext_matrix_get_translation_point3d (&model_matrix, &position);

  graphene_plane_init_from_point (res, &normal, &position);
}

float
xrd_window_get_aspect_ratio (XrdWindow *self)
{
  uint32_t w, h;
  g_object_get (self, "texture-width", &w, "texture-height", &h, NULL);
  return (float) w / (float) h;
}

/**
 * xrd_window_save_reset_transformation:
 * @self: The #XrdWindow
 * Saves the current transformation as the reset transformation.
  */
void
xrd_window_save_reset_transformation (XrdWindow *self)
{
  XrdWindowData *data = xrd_window_get_data (self);
  graphene_matrix_t current_transform;
  xrd_window_get_transformation_no_scale (self, &current_transform);
  graphene_matrix_init_from_matrix (&data->reset_transform,
                                    &current_transform);
}

/**
 * xrd_window_set_reset_transformation:
 * @self: The #XrdWindow
 * @transform: A transformation matrix to save as reset transform.
 */
void
xrd_window_set_reset_transformation (XrdWindow *self,
                                     graphene_matrix_t *transform)
{
  XrdWindowData *data = xrd_window_get_data (self);
  graphene_matrix_init_from_matrix (&data->reset_transform, transform);
}

void
xrd_window_get_reset_transformation (XrdWindow *self,
                                     graphene_matrix_t *transform)
{
  XrdWindowData *data = xrd_window_get_data (self);
  graphene_matrix_init_from_matrix (transform, &data->reset_transform);
}

/**
 * xrd_window_set_pin:
 * @self: The #XrdWindow
 * @pinned: The pin status to set this window to
 * @hide_unpinned: If TRUE, the window will be hidden if it is unpinned, and
 * shown if it is pinned. This corresponds to the "show only pinned windows"
 * mode set up in #XrdClient.
 * If FALSE, windows are always shown.
 * Note that @hide_unpinned only determines initial visibility, and does not
 * keep track of further mode changes.
 */
void
xrd_window_set_pin (XrdWindow *self,
                    gboolean pinned,
                    gboolean hide_unpinned)
{
  XrdWindowData *data = xrd_window_get_data (self);
  if (hide_unpinned)
    {
      if (pinned)
        xrd_window_show (self);
      else
        xrd_window_hide (self);
    }
  else
    xrd_window_show (self);

  data->pinned = pinned;
}

gboolean
xrd_window_is_pinned (XrdWindow *self)
{
  XrdWindowData *data = xrd_window_get_data (self);
  return data->pinned;
}

/**
 * xrd_window_close:
 * @self: The #XrdWindow
 * MUST be called when destroying a window to free its resources.
 */
void
xrd_window_close (XrdWindow *self)
{
  XrdWindowData *data = xrd_window_get_data (self);
  /* cleanup: If we are a child, we set our parent's child ptr to NULL */
  if (data->parent_window != NULL)
    data->parent_window->child_window = NULL;

  /* cleanup: If we are a parent, we set our childs parent ptr to NULL.
   * Usually we don't close parent windows while child windows still live,
   * but sometimes it can happen. */
  if (data->child_window)
    data->child_window->parent_window = NULL;
}
