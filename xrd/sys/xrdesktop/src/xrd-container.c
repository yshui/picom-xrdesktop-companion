/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-container.h"

#include <gxr.h>

#include "xrd-math.h"
#include "graphene-ext.h"

typedef struct {
  XrdWindow *window;
  graphene_matrix_t relative_transform;
} ContainerWindow;

struct _XrdContainer
{
  GObject parent;
  GSList *windows;
  float distance;

  graphene_matrix_t transform;
  GxrController *controller;

  XrdContainerAttachment attachment;
  XrdContainerLayout layout;
  gboolean visible;

  gint64 last_step_timestamp;
};

G_DEFINE_TYPE (XrdContainer, xrd_container, G_TYPE_OBJECT)

static void
xrd_container_finalize (GObject *gobject);

static void
xrd_container_class_init (XrdContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_container_finalize;
}

static void
xrd_container_init (XrdContainer *self)
{
  self->windows = NULL;
  self->distance = 0;
  self->layout = XRD_CONTAINER_VERTICAL;
  self->attachment = XRD_CONTAINER_ATTACHMENT_NONE;
  self->visible = TRUE;
  self->controller = NULL;
  self->last_step_timestamp = g_get_monotonic_time ();
}

/**
 * xrd_container_add_window:
 * @self: The container
 * @window: The window to add
 * @relative_transform: the transform of the window's center relative to the
 * container's center when XRD_CONTAINER_RELATIVE is used, ignored else (may
 * be NULL then).
 */
void
xrd_container_add_window (XrdContainer *self,
                          XrdWindow *window,
                          graphene_matrix_t *relative_transform)
{
  ContainerWindow *cw = g_malloc (sizeof (ContainerWindow));
  cw->window = window;
  if (relative_transform != NULL)
    graphene_matrix_init_from_matrix (&cw->relative_transform,
                                      relative_transform);

  self->windows = g_slist_append (self->windows, cw);

  /* initial positioning not important, will be overridden by attachment */
  graphene_matrix_init_identity (&self->transform);

  if (self->visible)
    xrd_window_show (window);
  else
    xrd_window_hide (window);
}

void
xrd_container_remove_window (XrdContainer *self,
                             XrdWindow *window)
{
  int index = g_slist_index (self->windows, window);
  if (index != -1)
    {
      self->windows = g_slist_remove (self->windows, window);
      self->windows = g_slist_remove (self->windows,
                                      g_slist_nth_data (self->windows,
                                                        (guint)index));
    }
}

void
xrd_container_set_distance (XrdContainer *self, float distance)
{
  self->distance = distance;
}

/**
 * xrd_container_get_windows:
 * @self: The container
 * Returns: A list of #XrdWindow contained in this container.
 * The list must be destroyed by the caller.
 */
GSList *
xrd_container_get_windows (XrdContainer *self)
{
  GSList *l = NULL;
  for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
    {
      ContainerWindow *cw = cwl->data;
      l = g_slist_append (l, cw->window);
    }

  return l;
}

float
xrd_container_get_distance (XrdContainer *self)
{
  return self->distance;
}

static void
_hmd_facing_pose (graphene_matrix_t *hmd_pose,
                  graphene_point3d_t *look_at_point_ws,
                  graphene_matrix_t *pose_ws)
{
  graphene_point3d_t hmd_location;
  graphene_ext_matrix_get_translation_point3d (hmd_pose, &hmd_location);

  graphene_point3d_t look_at_from_hmd = {
    .x = look_at_point_ws->x - hmd_location.x,
    .y = look_at_point_ws->y - hmd_location.y,
    .z = look_at_point_ws->z - hmd_location.z
  };

  graphene_vec3_t look_at_direction;
  graphene_point3d_to_vec3 (&look_at_from_hmd, &look_at_direction);

  float azimuth, inclination;
  xrd_math_get_rotation_angles (&look_at_direction, &azimuth, &inclination);

  graphene_matrix_init_identity (pose_ws);
  graphene_matrix_rotate_x (pose_ws, inclination);
  graphene_matrix_rotate_y (pose_ws, - azimuth);

  xrd_math_matrix_set_translation_point (pose_ws, look_at_point_ws);
}

static void
_window_container_set_transformation (XrdContainer *self,
                                      graphene_matrix_t *transform)
{
  float container_width = 0;
  float container_height = 0;

  for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
    {
      ContainerWindow *cw = cwl->data;
      XrdWindow *window = cw->window;
      if (self->layout == XRD_CONTAINER_VERTICAL)
        {
          container_height += xrd_window_get_current_height_meters (window);
          container_width =
            fmaxf (container_width,
                   xrd_window_get_current_width_meters (window));
        }
      else if (self->layout == XRD_CONTAINER_HORIZONTAL)
        {
          container_height =
            fmaxf (container_height,
                   xrd_window_get_current_height_meters (window));
          container_width += xrd_window_get_current_height_meters (window);
        }
    }

  /* How windows are placed:
   * Keep tally of left / top edge of unoccupied space in container in
   * x_offset/y_offset (-x left, +y up).
   *
   * In vertical layout:
   * Place window's center half the window height below the edge.
   * Then move the edge down the height of this window and repeat.
   * Finally, multiply with container's transform to place in world space.
   *
   * In horizontal layout: Same, but with width.
   */
  if (self->layout == XRD_CONTAINER_VERTICAL)
    {
      float y_offset = +container_height / 2.f;
      for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
        {
          ContainerWindow *cw = cwl->data;
          XrdWindow *window = cw->window;

          float window_height = xrd_window_get_current_height_meters (window);

          graphene_point3d_t xyoffset = {
            .x = 0.f,
            .y = y_offset - window_height / 2.f,
            .z = 0.f
          };
          graphene_matrix_t window_transform;
          graphene_matrix_init_translate (&window_transform, &xyoffset);
          graphene_matrix_multiply (&window_transform, transform,
                                    &window_transform);

          y_offset -= window_height;

          xrd_window_set_transformation (window, &window_transform);
        }
    }
  else if (self->layout == XRD_CONTAINER_HORIZONTAL)
    {
      float x_offset = -container_width / 2.f;
      for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
        {
          ContainerWindow *cw = cwl->data;
          XrdWindow *window = cw->window;

          float window_width = xrd_window_get_current_width_meters (window);

          graphene_point3d_t xyoffset = {
            .x = x_offset + window_width / 2.f,
            .y = 0.f,
            .z = 0.f
          };
          graphene_matrix_t window_transform;
          graphene_matrix_init_translate (&window_transform, &xyoffset);
          graphene_matrix_multiply (&window_transform, transform,
                                    &window_transform);

          x_offset += window_width;

          xrd_window_set_transformation (window, &window_transform);
        }
    }
  else if (self->layout == XRD_CONTAINER_RELATIVE)
    {
      for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
        {
          ContainerWindow *cw = cwl->data;
          XrdWindow *window = cw->window;

          graphene_matrix_t window_transform;
          graphene_matrix_multiply (&cw->relative_transform, transform,
                                    &window_transform);
          xrd_window_set_transformation (window, &window_transform);
        }
    }

  graphene_matrix_init_from_matrix (&self->transform, transform);
}

static gboolean
_step_fov (XrdContainer *self, GxrContext *context)
{
  /* Containers outside fov_factor_outer * FOV size will be snapped to
   * fov_factor_outer * FOV size.
   *
   * Containers between fov_factor_outer * FOV size and
   * fov_factor_inner * FOV size will smoothly move in center direction */
  const float fov_factor_outer = 0.6f;
  const float fov_factor_inner = 0.25f;

  graphene_matrix_t hmd_pose;
  gxr_context_get_head_pose (context, &hmd_pose);
  graphene_matrix_t hmd_pose_inv;
  graphene_matrix_inverse (&hmd_pose, &hmd_pose_inv);

  /* _cs means camera (hmd) space, _ws means world space. */
  graphene_matrix_t wc_transform_ws = self->transform;
  graphene_matrix_t wc_transform_cs;
  graphene_matrix_multiply (&wc_transform_ws, &hmd_pose_inv,
                            &wc_transform_cs);

  graphene_vec3_t wc_vec_cs;
  graphene_ext_matrix_get_translation_vec3 (&wc_transform_cs, &wc_vec_cs);

  float left, right, top, bottom;
  gxr_context_get_frustum_angles (context, GXR_EYE_LEFT,
                                  &left, &right, &top, &bottom);

  float left_inner = left * fov_factor_inner;
  float right_inner = right * fov_factor_inner;
  float top_inner = top * fov_factor_inner;
  float bottom_inner = bottom * fov_factor_inner;

  float left_outer = left * fov_factor_outer;
  float right_outer = right * fov_factor_outer;
  float top_outer = top * fov_factor_outer;
  float bottom_outer = bottom * fov_factor_outer;

  float radius = xrd_container_get_distance (self);

  /* azimuth: angle between view direction to window, "left-right" component.
   * inclination: angle between view directiont to window, "up-down" component.
   *
   * This reduces the problem in 3D space to a problem in 2D "angle space"
   * where azimuth and inclination are in [-180°,180°]x[-180°,180°].
   *
   * First the case where the window is completely out of view is handled:
   * Snapping the window towards the view direction is done by clamping
   * azimuth and inclination both towards the view direction angles (0, 0) until
   * any of the angles of the target FOV is hit.
   *
   * Then the case where the window is "too close" to being out of view is
   * handled: Angles describing a final target location for the window are
   * calculated by repeating the process towards a scaled down FOV, but it is
   * not snapped to the new position, but per frame moves proportional to the
   * remaining distance towards the target location.
   * */

  float azimuth, inclination;
  xrd_math_get_rotation_angles (&wc_vec_cs, &azimuth, &inclination);

  /* Bail early when the window already is in the "center area".
   * However still update the pose to reflect movement towards/away. */
  if (azimuth > left_inner && azimuth < right_inner &&
      inclination < top_inner && inclination > bottom_inner)
  {
    //g_print ("Not moving head following window!\n");
    graphene_point3d_t new_pos_ws;
    xrd_math_sphere_to_3d_coords (azimuth, inclination, radius, &new_pos_ws);
    graphene_matrix_transform_point3d (&hmd_pose, &new_pos_ws, &new_pos_ws);

    graphene_matrix_t new_wc_pose_ws;
    _hmd_facing_pose (&hmd_pose, &new_pos_ws, &new_wc_pose_ws);

    _window_container_set_transformation (self, &new_wc_pose_ws);
    return TRUE;
  }

  /* Window is not visible: snap it onto the edge of the visible area. */
  if (azimuth < left_outer || azimuth > right_outer ||
      inclination > top_outer || inclination < bottom_outer)
    {

      /* delta is used to snap windows a little closer towards the view center
       * because natural head movement doesn't suddenly stop, it slows down,
       * making it snap very small distances before it leaves the snap phase
       * and enters the smooth movement phase. This would make the transition
       * from slowing snapping to fast smooth movement look like a jump. */
      float delta = 1.0;
      graphene_point_t bottom_left = {
        .x = left_outer + delta,
        .y = bottom_outer + delta
      };
      graphene_point_t top_right = {
        .x = right_outer - delta,
        .y = top_outer - delta
      };
      graphene_point_t azimuth_inclination = { .x = azimuth, .y = inclination };

      graphene_point_t intersection_azimuth_inclination;
      gboolean intersects =
          xrd_math_clamp_towards_zero_2d (&bottom_left, &top_right,
                                          &azimuth_inclination,
                                          &intersection_azimuth_inclination);

      /* doesn't happen */
      if (!intersects)
        {
          g_print ("Head Following Window should intersect, but doesn't!\n");
          return TRUE;
        }

      graphene_point3d_t new_pos_ws;
      xrd_math_sphere_to_3d_coords (intersection_azimuth_inclination.x,
                                    intersection_azimuth_inclination.y, radius,
                                    &new_pos_ws);
      graphene_matrix_transform_point3d (&hmd_pose, &new_pos_ws, &new_pos_ws);

      graphene_matrix_t new_wc_pose_ws;
      _hmd_facing_pose (&hmd_pose, &new_pos_ws, &new_wc_pose_ws);

      _window_container_set_transformation (self, &new_wc_pose_ws);


      graphene_vec2_t velocity;
      graphene_vec2_init (&velocity,
                          inclination - intersection_azimuth_inclination.y,
                          azimuth - intersection_azimuth_inclination.x);

      //g_print ("Snap window to view frustum edge!\n");
      return TRUE;
    }


  /* Window is visible, but not in center area: move it towards center area*/
  graphene_point_t bottom_left = { .x = left_inner, .y = bottom_inner };
  graphene_point_t top_right = { .x = right_inner, .y = top_inner };
  graphene_point_t azimuth_inclination = { .x = azimuth, .y = inclination };

  graphene_point_t intersection_azimuth_inclination;
  gboolean intersects =
    xrd_math_clamp_towards_zero_2d (&bottom_left, &top_right,
                                    &azimuth_inclination,
                                    &intersection_azimuth_inclination);

  /* doesn't happen */
  if (!intersects)
    {
      g_print ("Head Following Window should intersect, but doesn't!\n");
      return TRUE;
    }

  float azimuth_diff = azimuth - intersection_azimuth_inclination.x;
  float inclination_diff = inclination - intersection_azimuth_inclination.y;

  float ms_since_last_step =
    (float) (g_get_monotonic_time () - self->last_step_timestamp) / 1000.f;

  /* Container moves by a fixed fraction of the distance from current point
   * to target point, i.e. on a linear deceleration curve. */
  graphene_vec2_t angle_velocity;
  graphene_vec2_init (&angle_velocity, azimuth_diff, inclination_diff);
  float remaining_angle_distance = graphene_vec2_length (&angle_velocity);
  float distance_speed_factor = ms_since_last_step / 1000.f * 7.f;
  float angle_speed = remaining_angle_distance * distance_speed_factor;


  graphene_vec2_normalize (&angle_velocity, &angle_velocity);
  graphene_vec2_scale (&angle_velocity, angle_speed, &angle_velocity);

  graphene_vec2_t current_angles;
  graphene_vec2_init (&current_angles, azimuth, inclination);

  graphene_vec2_t next_angles;
  graphene_vec2_subtract (&current_angles, &angle_velocity, &next_angles);

  graphene_point3d_t next_point_cs;
  xrd_math_sphere_to_3d_coords (graphene_vec2_get_x (&next_angles),
                                graphene_vec2_get_y (&next_angles),
                                radius, &next_point_cs);

  graphene_point3d_t next_point_ws;
  graphene_matrix_transform_point3d (&hmd_pose, &next_point_cs, &next_point_ws);

  graphene_matrix_t new_wc_pose_ws;
  _hmd_facing_pose (&hmd_pose, &next_point_ws, &new_wc_pose_ws);
  _window_container_set_transformation (self, &new_wc_pose_ws);

  //g_print ("Moving head following window with %f!\n", angle_speed);
  return TRUE;
}

static gboolean
_step_hand (XrdContainer *self)
{
  if (!G_IS_OBJECT (self->controller))
    {
      /* controller is turned off */
      self->controller = NULL;
      return FALSE;
    }


  graphene_matrix_t container_transform;
  /* hand_grip "lays" in xz plane, but window is in xy plane. */
  graphene_matrix_init_rotate (&container_transform, -80,
                               graphene_vec3_x_axis ());

  graphene_point3d_t offset = { .x = .0f, .y = .050f, .z = -.033f };
  graphene_matrix_translate (&container_transform, &offset);

  graphene_matrix_t controller_pose;
  gxr_controller_get_hand_grip_pose (self->controller, &controller_pose);

  graphene_matrix_multiply (&container_transform,
                            &controller_pose,
                            &container_transform);

  _window_container_set_transformation (self, &container_transform);
  return TRUE;
}

/**
 * xrd_container_step:
 * @self: The #XrdContainer
 * @context: A #GxrContext
 *
 * Updates the container's position based on its attachment.
 *
 * Returns: A #gboolean if that is %TRUE the step was successful.
 */
gboolean
xrd_container_step (XrdContainer *self, GxrContext *context)
{
  gboolean ret = FALSE;
  switch (self->attachment)
  {
    case (XRD_CONTAINER_ATTACHMENT_HEAD):
    {
      ret = _step_fov (self, context);
      break;
    }
    case (XRD_CONTAINER_ATTACHMENT_HAND):
    {
      ret = _step_hand (self);
      break;
    }
    case (XRD_CONTAINER_ATTACHMENT_NONE):
    {
      ret = TRUE;
      break;
    }
  }

  self->last_step_timestamp = g_get_monotonic_time ();
  return ret;
}

/**
 * xrd_container_set_attachment:
 * @self: The container.
 * @attachment: The attachment to set.
 * @controller: A controller used for XRD_CONTAINER_ATTACHMENT_HAND. May be
 * NULL for other attachments.
 */
void
xrd_container_set_attachment (XrdContainer *self,
                              XrdContainerAttachment attachment,
                              GxrController *controller)
{
  self->attachment = attachment;
  self->controller = controller;

  switch (attachment)
  {
    case (XRD_CONTAINER_ATTACHMENT_HEAD):
    {
      break;
    }
    case (XRD_CONTAINER_ATTACHMENT_HAND):
    {
      break;
    }
    case (XRD_CONTAINER_ATTACHMENT_NONE):
    {
      break;
    }
  }
}

void
xrd_container_set_layout (XrdContainer *self,
                          XrdContainerLayout layout)
{
  self->layout = layout;
}

void
xrd_container_hide (XrdContainer *self)
{
  for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
    {
      ContainerWindow *cw = cwl->data;
      XrdWindow *window = cw->window;
      xrd_window_hide (window);
    }
  self->visible = FALSE;
}

void
xrd_container_show (XrdContainer *self)
{
  for (GSList *cwl = self->windows; cwl; cwl = cwl->next)
    {
      ContainerWindow *cw = cwl->data;
      XrdWindow *window = cw->window;
      xrd_window_show (window);
    }
  self->visible = TRUE;
}

gboolean
xrd_container_is_visible (XrdContainer *self)
{
  return self->visible;
}

/**
 * xrd_container_center_view:
 * @self: The container.
 * @context: A #GxrContext
 * @distance: The distance from the HMD the container should have.
 *
 * Places the container in the center of the FOV at the given distance.
 */
void
xrd_container_center_view (XrdContainer *self,
                           GxrContext *context,
                           float distance)
{
  graphene_matrix_t hmd_transform;
  if (!gxr_context_get_head_pose (context, &hmd_transform))
    {
      g_printerr ("Could not get head pose.\n");
      return;
    }

  graphene_point3d_t distance_point = {
      .x = 0,
      .y = 0,
      .z = -distance
  };

  graphene_matrix_t container_transform;
  graphene_matrix_init_translate (&container_transform, &distance_point);

  graphene_matrix_multiply (&container_transform, &hmd_transform,
                            &container_transform);

  _window_container_set_transformation (self, &container_transform);
}

XrdContainer *
xrd_container_new (void)
{
  return (XrdContainer*) g_object_new (XRD_TYPE_CONTAINER, 0);
}

static void
xrd_container_finalize (GObject *gobject)
{
  XrdContainer *self = XRD_CONTAINER (gobject);
  g_slist_free_full (self->windows, g_free);
  (void) self;
}
