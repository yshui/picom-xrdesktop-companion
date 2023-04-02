/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_CONTROLLER_H_
#define GXR_CONTROLLER_H_

#include <glib-object.h>

#include "gxr-device.h"

#include "gxr-pointer-tip.h"
#include "gxr-pointer.h"
#include <graphene.h>

G_BEGIN_DECLS

#define GXR_TYPE_CONTROLLER gxr_controller_get_type()
G_DECLARE_FINAL_TYPE (GxrController, gxr_controller, GXR, CONTROLLER, GxrDevice)

/**
 * GxrTransformLock:
 * @GXR_TRANSFORM_LOCK_NONE: The grab action does not currently have a transformation it is locked to.
 * @GXR_TRANSFORM_LOCK_PUSH_PULL: Only push pull transformation can be performed.
 * @GXR_TRANSFORM_LOCK_SCALE: Only a scale transformation can be performed.
 *
 * The type of transformation the grab action is currently locked to.
 * This will be detected at the begginging of a grab transformation
 * and reset after the transformation is done.
 *
 **/
typedef enum {
  GXR_TRANSFORM_LOCK_NONE,
  GXR_TRANSFORM_LOCK_PUSH_PULL,
  GXR_TRANSFORM_LOCK_SCALE
} GxrTransformLock;

typedef struct {
  gpointer          hovered_object;
  float             distance;
  graphene_point_t  intersection_2d;
} GxrHoverState;

typedef struct {
  gpointer              grabbed_object;
  graphene_quaternion_t object_rotation;
  graphene_quaternion_t inverse_controller_rotation;
  graphene_point_t      grab_offset;
  GxrTransformLock      transform_lock;
} GxrGrabState;

GxrController *gxr_controller_new (guint64 controller_handle,
                                   GxrContext *context,
                                   gchar      *model_name);

GxrPointer *
gxr_controller_get_pointer (GxrController *self);

GxrPointerTip *
gxr_controller_get_pointer_tip (GxrController *self);

void
gxr_controller_set_pointer (GxrController *self, GxrPointer *pointer);

void
gxr_controller_set_pointer_tip (GxrController *self, GxrPointerTip *tip);

GxrHoverState *
gxr_controller_get_hover_state (GxrController *self);

GxrGrabState *
gxr_controller_get_grab_state (GxrController *self);

void
gxr_controller_reset_grab_state (GxrController *self);

void
gxr_controller_reset_hover_state (GxrController *self);

void
gxr_controller_get_hand_grip_pose (GxrController *self,
                                   graphene_matrix_t *pose);

void
gxr_controller_update_pointer_pose (GxrController     *self,
                                    graphene_matrix_t *pose,
                                    gboolean           valid);

void
gxr_controller_update_hand_grip_pose (GxrController     *self,
                                      graphene_matrix_t *pose,
                                      gboolean           valid);

gboolean
gxr_controller_is_pointer_pose_valid (GxrController *self);

void
gxr_controller_hide_pointer (GxrController *self);

void
gxr_controller_show_pointer (GxrController *self);

gboolean
gxr_controller_is_pointer_visible (GxrController *self);

float
gxr_controller_get_distance (GxrController *self, graphene_point3d_t *point);

gboolean
gxr_controller_get_pointer_pose (GxrController *self, graphene_matrix_t *pose);

void
gxr_controller_update_hovered_object (GxrController *self,
                                      gpointer last_object,
                                      gpointer object,
                                      graphene_matrix_t *object_pose,
                                      graphene_point3d_t *intersection_point,
                                      graphene_point_t *intersection_2d,
                                      float intersection_distance);

void
gxr_controller_drag_start (GxrController *self,
                           gpointer grabbed_object,
                           graphene_matrix_t *object_pose);

gboolean
gxr_controller_get_drag_pose (GxrController     *self,
                              graphene_matrix_t *drag_pose);

void
gxr_controller_set_user_data (GxrController *self, gpointer data);

gpointer
gxr_controller_get_user_data (GxrController *self);

G_END_DECLS

#endif /* GXR_CONTROLLER_H_ */
