/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-controller.h"
#include "gxr-pointer.h"

#include "graphene-ext.h"
#include "gxr-device.h"

struct _GxrController
{
  GxrDevice parent;

  GxrPointer *pointer_ray;
  GxrPointerTip *pointer_tip;
  GxrHoverState hover_state;
  GxrGrabState grab_state;

  graphene_matrix_t pointer_pose;
  gboolean pointer_pose_valid;

  graphene_matrix_t hand_grip_pose;
  gboolean hand_grip_pose_valid;

  graphene_matrix_t intersection_pose;
  GxrContext *context;

  gpointer user_data;
};

G_DEFINE_TYPE (GxrController, gxr_controller, GXR_TYPE_DEVICE)

static void
gxr_controller_finalize (GObject *gobject);

static void
gxr_controller_class_init (GxrControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_controller_finalize;
}

static void
gxr_controller_init (GxrController *self)
{
  self->hover_state.distance = 1.0f;
  self->grab_state.transform_lock = GXR_TRANSFORM_LOCK_NONE;

  self->grab_state.grabbed_object = NULL;
  self->hover_state.hovered_object = NULL;

  graphene_matrix_init_identity (&self->pointer_pose);
  graphene_matrix_init_identity (&self->hand_grip_pose);
  self->pointer_ray = NULL;
  self->pointer_tip = NULL;

  self->pointer_pose_valid = FALSE;
  self->hand_grip_pose_valid = FALSE;

  self->context = NULL;

  self->user_data = NULL;
}

GxrController *
gxr_controller_new (guint64 controller_handle,
                    GxrContext *context,
                    gchar *model_name)
{
  GxrController *controller =
    (GxrController*) g_object_new (GXR_TYPE_CONTROLLER, 0);

  GxrDevice *device = GXR_DEVICE (controller);
  gxr_device_set_handle (device, controller_handle);

  controller->context = context;

  gxr_device_set_model_name (GXR_DEVICE (controller), model_name);

  return controller;
}

static void
gxr_controller_finalize (GObject *gobject)
{
  GxrController *self = GXR_CONTROLLER (gobject);
  if (self->pointer_ray)
    g_object_unref (self->pointer_ray);
  if (self->pointer_tip)
    g_object_unref (self->pointer_tip);

  g_debug ("Destroyed pointer ray, pointer tip, controller\n");

  G_OBJECT_CLASS (gxr_controller_parent_class)->finalize (gobject);
}

GxrPointer *
gxr_controller_get_pointer (GxrController *self)
{
  return self->pointer_ray;
}

GxrPointerTip *
gxr_controller_get_pointer_tip (GxrController *self)
{
  return self->pointer_tip;
}

void
gxr_controller_set_pointer (GxrController *self, GxrPointer *pointer)
{
  self->pointer_ray = pointer;
}

void
gxr_controller_set_pointer_tip (GxrController *self, GxrPointerTip *tip)
{
  self->pointer_tip = tip;
}

GxrHoverState *
gxr_controller_get_hover_state (GxrController *self)
{
  return &self->hover_state;
}

GxrGrabState *
gxr_controller_get_grab_state (GxrController *self)
{
  return &self->grab_state;
}

void
gxr_controller_reset_grab_state (GxrController *self)
{
  self->grab_state.grabbed_object = NULL;
  graphene_point_init (&self->grab_state.grab_offset, 0, 0);
  graphene_quaternion_init_identity (
    &self->grab_state.inverse_controller_rotation);
  self->grab_state.transform_lock = GXR_TRANSFORM_LOCK_NONE;
}

void
gxr_controller_reset_hover_state (GxrController *self)
{
  self->hover_state.hovered_object = NULL;
  graphene_point_init (&self->hover_state.intersection_2d, 0, 0);
  self->hover_state.distance = 1.0;
}

void
gxr_controller_get_hand_grip_pose (GxrController *self,
                                   graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (pose, &self->hand_grip_pose);
}

static void
_no_hover_transform_tip (GxrController *self,
                         graphene_matrix_t *controller_pose)
{
  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point,
                         0.f,
                         0.f,
                         -5.0f);

  graphene_matrix_t tip_pose;

  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation, controller_pose);

  graphene_point3d_t controller_translation_point;
  graphene_ext_matrix_get_translation_point3d (controller_pose,
                                               &controller_translation_point);

  graphene_matrix_init_identity (&tip_pose);
  graphene_matrix_translate (&tip_pose, &distance_translation_point);
  graphene_matrix_rotate_quaternion (&tip_pose, &controller_rotation);
  graphene_matrix_translate (&tip_pose, &controller_translation_point);

  gxr_pointer_tip_set_transformation (self->pointer_tip, &tip_pose);

  gxr_pointer_tip_update_apparent_size (self->pointer_tip, self->context);

  gxr_pointer_tip_set_active (self->pointer_tip, FALSE);
}

void
gxr_controller_update_pointer_pose (GxrController     *self,
                                    graphene_matrix_t *pose,
                                    gboolean           valid)
{
  graphene_matrix_init_from_matrix (&self->pointer_pose, pose);

  /* when hovering or grabbing, the tip's transform is controlled by
   * gxr_controller_update_hovered_object() because the tip is oriented
   * towards the surface it is on, which we have not stored in hover_state. */
  if (self->hover_state.hovered_object == NULL &&
    self->grab_state.grabbed_object == NULL)
    _no_hover_transform_tip (self, pose);

  /* The pointer's pose is always set by pose update. When hovering, only the
   * length is set by gxr_controller_update_hovered_object(). */
  gxr_pointer_move (self->pointer_ray, pose);

  self->pointer_pose_valid = valid;
}

void
gxr_controller_update_hand_grip_pose (GxrController     *self,
                                      graphene_matrix_t *pose,
                                      gboolean           valid)
{
  graphene_matrix_init_from_matrix (&self->hand_grip_pose, pose);
  self->hand_grip_pose_valid = valid;
}

gboolean
gxr_controller_is_pointer_pose_valid (GxrController *self)
{
  return self->pointer_pose_valid;
}

void
gxr_controller_hide_pointer (GxrController *self)
{
  gxr_pointer_hide (self->pointer_ray);
  gxr_pointer_tip_hide (self->pointer_tip);
}

void
gxr_controller_show_pointer (GxrController *self)
{
  gxr_pointer_show (self->pointer_ray);
  gxr_pointer_tip_show (self->pointer_tip);
}

gboolean
gxr_controller_is_pointer_visible (GxrController *self)
{
  return gxr_pointer_tip_is_visible (self->pointer_tip);
}

static float
_point_matrix_distance (graphene_point3d_t *intersection_point,
                        graphene_matrix_t  *pose)
{
  graphene_vec3_t intersection_vec;
  graphene_point3d_to_vec3 (intersection_point, &intersection_vec);

  graphene_vec3_t pose_translation;
  graphene_ext_matrix_get_translation_vec3 (pose, &pose_translation);

  graphene_vec3_t distance_vec;
  graphene_vec3_subtract (&pose_translation,
                          &intersection_vec,
                          &distance_vec);

  return graphene_vec3_length (&distance_vec);
}

float
gxr_controller_get_distance (GxrController *self, graphene_point3d_t *point)
{
  return _point_matrix_distance (point, &self->pointer_pose);
}

gboolean
gxr_controller_get_pointer_pose (GxrController *self, graphene_matrix_t *pose)
{
  graphene_matrix_init_from_matrix (pose, &self->pointer_pose);
  return self->pointer_pose_valid;
}

void
gxr_controller_update_hovered_object (GxrController *self,
                                      gpointer last_object,
                                      gpointer object,
                                      graphene_matrix_t *object_pose,
                                      graphene_point3d_t *intersection_point,
                                      graphene_point_t *intersection_2d,
                                      float intersection_distance)
{
  if (last_object != object)
    gxr_pointer_tip_set_active (self->pointer_tip, object != NULL);

  if (object)
    {
      self->hover_state.hovered_object = object;

      graphene_point_init_from_point (&self->hover_state.intersection_2d,
                                      intersection_2d);

      self->hover_state.distance = intersection_distance;

      gxr_pointer_tip_update (self->pointer_tip, self->context,
                              object_pose, intersection_point);

      gxr_pointer_set_length (self->pointer_ray, intersection_distance);
    }
  else
    {
      graphene_quaternion_t controller_rotation;
      graphene_quaternion_init_from_matrix (&controller_rotation,
                                            &self->pointer_pose);

      graphene_point3d_t distance_point;
      graphene_point3d_init (&distance_point,
                            0.f,
                            0.f,
                            -gxr_pointer_get_default_length (self->pointer_ray));

      graphene_point3d_t controller_position;
      graphene_ext_matrix_get_translation_point3d (&self->pointer_pose,
                                                  &controller_position);

      graphene_matrix_t tip_pose;
      graphene_matrix_init_identity (&tip_pose);
      graphene_matrix_translate (&tip_pose, &distance_point);
      graphene_matrix_rotate_quaternion (&tip_pose, &controller_rotation);
      graphene_matrix_translate (&tip_pose, &controller_position);
      gxr_pointer_tip_set_transformation (self->pointer_tip, &tip_pose);
      gxr_pointer_tip_update_apparent_size (self->pointer_tip, self->context);


      if (last_object)
        {
          gxr_pointer_reset_length (self->pointer_ray);
          gxr_controller_reset_hover_state (self);
          gxr_controller_reset_grab_state (self);
        }
    }

  /* TODO:
  gxr_pointer_set_selected_object (self->pointer_ray, object);
  */
}

void
gxr_controller_drag_start (GxrController *self,
                           gpointer grabbed_object,
                           graphene_matrix_t *object_pose)
{
  self->grab_state.grabbed_object = grabbed_object;

  graphene_quaternion_t controller_rotation;
  graphene_ext_matrix_get_rotation_quaternion (&self->pointer_pose,
                                               &controller_rotation);

  graphene_ext_matrix_get_rotation_quaternion (object_pose,
                                               &self->grab_state.object_rotation);

  graphene_point_init (
    &self->grab_state.grab_offset,
    -self->hover_state.intersection_2d.x,
    -self->hover_state.intersection_2d.y);

  graphene_quaternion_invert (
    &controller_rotation,
    &self->grab_state.inverse_controller_rotation);
}

gboolean
gxr_controller_get_drag_pose (GxrController     *self,
                              graphene_matrix_t *drag_pose)
{
  if (self->grab_state.grabbed_object == NULL)
    return FALSE;

  graphene_point3d_t controller_translation_point;
  graphene_ext_matrix_get_translation_point3d (&self->pointer_pose,
                                               &controller_translation_point);
  graphene_quaternion_t controller_rotation;
  graphene_quaternion_init_from_matrix (&controller_rotation,
                                        &self->pointer_pose);

  graphene_point3d_t distance_translation_point;
  graphene_point3d_init (&distance_translation_point,
                         0.f, 0.f, -self->hover_state.distance);

  /* Build a new transform for pointer tip in event->pose.
   * Pointer tip is at intersection, in the plane of the window,
   * so we can reuse the tip rotation for the window rotation. */
  graphene_matrix_init_identity (&self->intersection_pose);

  /* restore original rotation of the tip */
  graphene_matrix_rotate_quaternion (&self->intersection_pose,
                                     &self->grab_state.object_rotation);

  /* Later the current controller rotation is applied to the overlay, so to
   * keep the later controller rotations relative to the initial controller
   * rotation, rotate the window in the opposite direction of the initial
   * controller rotation.
   * This will initially result in the same window rotation so the window does
   * not change its rotation when being grabbed, and changing the controllers
   * position later will rotate the window with the "diff" of the controller
   * rotation to the initial controller rotation. */
  graphene_matrix_rotate_quaternion (
    &self->intersection_pose, &self->grab_state.inverse_controller_rotation);

  /* then translate the overlay to the controller ray distance */
  graphene_matrix_translate (&self->intersection_pose,
                             &distance_translation_point);

  /* Rotate the translated overlay to where the controller is pointing. */
  graphene_matrix_rotate_quaternion (&self->intersection_pose,
                                     &controller_rotation);

  /* Calculation was done for controller in (0,0,0), just move it with
   * controller's offset to real (0,0,0) */
  graphene_matrix_translate (&self->intersection_pose,
                             &controller_translation_point);



  graphene_matrix_t transformation_matrix;
  graphene_matrix_init_identity (&transformation_matrix);

  graphene_point3d_t grab_offset3d;
  graphene_point3d_init (&grab_offset3d,
                         self->grab_state.grab_offset.x,
                         self->grab_state.grab_offset.y,
                         0);

  /* translate such that the grab point is pivot point. */
  graphene_matrix_translate (&transformation_matrix, &grab_offset3d);

  /* window has the same rotation as the tip we calculated in event->pose */
  graphene_matrix_multiply (&transformation_matrix,
                            &self->intersection_pose,
                            &transformation_matrix);

  graphene_matrix_init_from_matrix (drag_pose, &transformation_matrix);

  /* TODO:
  XrdPointer *pointer = self->pointer_ray;
  xrd_pointer_set_selected_window (pointer, window);
  */

  gxr_pointer_tip_set_transformation (self->pointer_tip,
                                      &self->intersection_pose);
  /* update apparent size after pointer has been moved */
  gxr_pointer_tip_update_apparent_size (self->pointer_tip, self->context);

  return TRUE;
}

void
gxr_controller_set_user_data (GxrController *self, gpointer data)
{
  self->user_data = data;
}

gpointer
gxr_controller_get_user_data (GxrController *self)
{
  return self->user_data;
}
