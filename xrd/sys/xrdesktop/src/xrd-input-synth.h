/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_INPUT_SYNTH_H_
#define XRD_INPUT_SYNTH_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include <gxr.h>

#include "xrd-window.h"

G_BEGIN_DECLS

#define XRD_TYPE_INPUT_SYNTH xrd_input_synth_get_type()
G_DECLARE_FINAL_TYPE (XrdInputSynth, xrd_input_synth, XRD, INPUT_SYNTH, GObject)

typedef enum {
  LEFT_BUTTON = 1,
  MIDDLE_BUTTON = 2,
  RIGHT_BUTTON = 3,

  SCROLL_UP = 4,
  SCROLL_DOWN = 5,
  SCROLL_LEFT  = 6,
  SCROLL_RIGHT = 7
} InputSynthButton;

/**
 * XrdClickEvent:
 * @window: The #XrdWindow that was clicked.
 * @position: A #graphene_point_t 2D screen position for the click.
 * @button: The #int identifier of the mouse button.
 * @state: A #gboolean that is %TRUE when pressed and %FALSE when released.
 * @controller_handle: A #guint64 with the OpenVR handle to the controller.
 *
 * A 2D mouse click event.
 **/
typedef struct {
  XrdWindow        *window;
  graphene_point_t *position;
  InputSynthButton  button;
  gboolean          state;
  GxrController    *controller;
} XrdClickEvent;

/**
 * XrdMoveCursorEvent:
 * @window: The #XrdWindow on which the cursor was moved.
 * @position: A #graphene_point_t with the current 2D screen position.
 * @ignore: A #gboolean wheather the synthesis should be ignored.
 *
 * A 2D mouse move event.
 *
 * Ignoring this events means only updating the cursor position in VR so it
 * does not appear frozen, but don't actually synthesize mouse move events.
 *
 **/
typedef struct {
  XrdWindow *window;
  graphene_point_t *position;
  gboolean ignore;
} XrdMoveCursorEvent;

XrdInputSynth *
xrd_input_synth_new (void);

void
xrd_input_synth_reset_scroll (XrdInputSynth *self);

void
xrd_input_synth_reset_press_state (XrdInputSynth *self);

void
xrd_input_synth_move_cursor (XrdInputSynth    *self,
                             XrdWindow *window,
                             graphene_matrix_t *controller_pose,
                             graphene_point3d_t *intersection);

GxrController *
xrd_input_synth_get_primary_controller (XrdInputSynth *self);

void
xrd_input_synth_make_primary (XrdInputSynth *self,
                              GxrController *controller);

GxrActionSet*
xrd_input_synth_create_action_set (XrdInputSynth *self, GxrContext *context);

G_END_DECLS

#endif /* XRD_INPUT_SYNTH_H_ */
