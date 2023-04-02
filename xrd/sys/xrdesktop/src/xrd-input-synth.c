/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-input-synth.h"

#include <gdk/gdk.h>

#include "xrd-settings.h"
#include "graphene-ext.h"
#include "xrd-shake-compensator.h"

struct _XrdInputSynth
{
  GObject parent;

  /* hover_position is relative to hover_window */
  graphene_point_t hover_position;
  XrdWindow *hover_window;

  /* bitmask */
  uint32_t button_press_state;

  graphene_vec3_t scroll_accumulator;

  float scroll_threshold;

  GxrController *primary_controller;

  XrdShakeCompensator *compensator;
  gboolean compensator_enabled;
};

G_DEFINE_TYPE (XrdInputSynth, xrd_input_synth, G_TYPE_OBJECT)

enum {
  CLICK_EVENT,
  MOVE_CURSOR_EVENT,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
xrd_input_synth_finalize (GObject *gobject);

static void
xrd_input_synth_class_init (XrdInputSynthClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_input_synth_finalize;

  signals[CLICK_EVENT] =
    g_signal_new ("click-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  signals[MOVE_CURSOR_EVENT] =
    g_signal_new ("move-cursor-event",
                   G_TYPE_FROM_CLASS (klass),
                   G_SIGNAL_RUN_LAST,
                   0, NULL, NULL, NULL, G_TYPE_NONE,
                   1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

XrdInputSynth *
xrd_input_synth_new (void)
{
  return (XrdInputSynth*) g_object_new (XRD_TYPE_INPUT_SYNTH, 0);
}

static void
xrd_input_synth_finalize (GObject *gobject)
{
  XrdInputSynth *self = XRD_INPUT_SYNTH (gobject);
  g_object_unref (self->compensator);

  G_OBJECT_CLASS (xrd_input_synth_parent_class)->finalize (gobject);
}

static gboolean
_is_compensate_shake (InputSynthButton button)
{
  return
    button == LEFT_BUTTON ||
    button == RIGHT_BUTTON ||
    button == MIDDLE_BUTTON;
}

static void
_emit_click (XrdInputSynth    *self,
             graphene_point_t *position,
             InputSynthButton  button,
             gboolean          state)
{
  /* Button press and release only start and stop prediction.
   * If necessary, the prediction queue is replayed in mouse move. */
  if (state && _is_compensate_shake (button) && self->compensator_enabled)
    xrd_shake_compensator_start_recording (self->compensator, button);
  else if (!state && button ==
           xrd_shake_compensator_get_button (self->compensator))
    xrd_shake_compensator_reset (self->compensator);

  XrdClickEvent *click_event = g_malloc (sizeof (XrdClickEvent));
  click_event->position = position;
  click_event->button = (InputSynthButton)button;
  click_event->state = state;
  click_event->controller = self->primary_controller;
  g_signal_emit (self, signals[CLICK_EVENT], 0, click_event);
}

/**
 * xrd_input_synth_reset_press_state:
 * @self: The #XrdInputSynth
 *
 * Issue a button release event for every button that previously was used for a
 * button press event, but has not been released yet.
 *
 * When calling this function, also consider xrd_input_synth_reset_scroll().
 */
void
xrd_input_synth_reset_press_state (XrdInputSynth *self)
{
  if (self && self->button_press_state)
    {
      /*g_print ("End hover, button mask %d...\n", self->button_press_state); */
      for (InputSynthButton button = 1; button <= 8; button++)
        {
          gboolean pressed = (self->button_press_state & 1 << button) != 0;
          if (pressed)
            {
              g_print ("Released button %d\n", button);
              _emit_click (self, &self->hover_position, button, FALSE);
            }
        }
      self->button_press_state = 0;
    }
}

static void
_process_click (XrdInputSynth   *self,
                GxrDigitalEvent *event,
                InputSynthButton button)
{
  if (self->primary_controller != event->controller)
    {
      g_free (event);
      return;
    }

  if (event->changed)
    {
      _emit_click (self, &self->hover_position,
                   button, event->state);

      if (event->state)
        self->button_press_state |= 1 << button;
      else
        self->button_press_state &= ~(1 << button);
    }
  g_free (event);
}

static void
_action_left_click_cb (GxrAction       *action,
                       GxrDigitalEvent *event,
                       XrdInputSynth   *self)
{
  (void) action;

  /* when left clicking with a controller that is *not* used to do input
   * synth, make this controller do input synth */
  if (event->state && self->primary_controller != event->controller)
    {
      g_free (event);
      xrd_input_synth_make_primary (self, event->controller);
      return;
    }

  _process_click (self, event, LEFT_BUTTON);
}

static void
_action_right_click_cb (GxrAction       *action,
                        GxrDigitalEvent *event,
                        XrdInputSynth   *self)
{
  (void) action;
  _process_click (self, event, RIGHT_BUTTON);
}

static void
_action_middle_click_cb (GxrAction       *action,
                         GxrDigitalEvent *event,
                         XrdInputSynth   *self)
{
  (void) action;
  _process_click (self, event, MIDDLE_BUTTON);
}

static void
_do_scroll (XrdInputSynth *self, int steps_x, int steps_y)
{
  for (int i = 0; i < abs(steps_y); i++)
    {
      InputSynthButton btn;
      if (steps_y > 0)
        btn = SCROLL_UP;
      else
        btn = SCROLL_DOWN;
      _emit_click (self, &self->hover_position, btn, TRUE);
      _emit_click (self, &self->hover_position, btn, FALSE);
    }

  for (int i = 0; i < abs(steps_x); i++)
    {
      InputSynthButton btn;
      if (steps_x < 0)
        btn = SCROLL_LEFT;
      else
        btn = SCROLL_RIGHT;
      _emit_click (self, &self->hover_position, btn, TRUE);
      _emit_click (self, &self->hover_position, btn, FALSE);
    }
}

/*
 * When the touchpad is touched, start adding up movement.
 * If movement is over threshold, create a scroll event and reset
 * scroll_accumulator.
 */

static void
_action_scroll_cb (GxrAction      *action,
                   GxrAnalogEvent *event,
                   XrdInputSynth  *self)
{
  (void) action;

  if (self->primary_controller != event->controller)
    {
      g_free (event);
      return;
    }
  
  static graphene_vec3_t last_touch_pos;
  gboolean initial_touch = graphene_vec3_get_x (&last_touch_pos) == 0.0f &&
                           graphene_vec3_get_y (&last_touch_pos) == 0.0f;
  graphene_vec3_init_from_vec3 (&last_touch_pos, &event->state);

  /* When stopping to touch the touchpad we get a deltea from where the
   * touchpad is touched to (0,0). Ignore this bogus delta. */
  if (graphene_vec3_get_x (&event->state) == 0.0f &&
      graphene_vec3_get_y (&event->state) == 0.0f)
    {
      g_free(event);
      return;
    }

  /* When starting to touch the touchpad, we get a delta from (0,0) to where
   * the touchpad is touched. Ignore this bogus delta. */
  if (initial_touch)
    {
      g_free (event);
      return;
    }

  graphene_vec3_add (&self->scroll_accumulator, &event->delta,
                     &self->scroll_accumulator);

  float x_acc = graphene_vec3_get_x (&self->scroll_accumulator);
  float y_acc = graphene_vec3_get_y (&self->scroll_accumulator);

  /*
   * Scroll as many times as the threshold has been exceeded.
   * e.g. user scrolled 0.32 with threshold of 0.1 -> scroll 3 times.
   */
  int steps_x = (int) (x_acc / self->scroll_threshold);
  int steps_y = (int) (y_acc / self->scroll_threshold);

  /*
   * We need to keep the rest in the accumulator to not lose part of the
   * user's movement e.g. 0.32: -> 0.2 and -0.32 -> -0.2
   */
  float rest_x = x_acc - (float)steps_x * self->scroll_threshold;
  float rest_y = y_acc - (float)steps_y * self->scroll_threshold;
  graphene_vec3_init (&self->scroll_accumulator, rest_x, rest_y, 0);

  _do_scroll (self, steps_x, steps_y);

  g_free (event);
}

void
xrd_input_synth_move_cursor (XrdInputSynth    *self,
                             XrdWindow *window,
                             graphene_matrix_t *controller_pose,
                             graphene_point3d_t *intersection)
{
  graphene_point_t intersection_pixels;
  xrd_window_get_intersection_2d_pixels (window, intersection,
                                        &intersection_pixels);
  
  XrdMoveCursorEvent *event = g_malloc (sizeof (XrdMoveCursorEvent));
  event->window = window;
  event->position = &intersection_pixels;
  event->ignore = FALSE;

  graphene_point_init_from_point (&self->hover_position, &intersection_pixels);
  self->hover_window = window;

  if (xrd_shake_compensator_is_recording (self->compensator))
    {
      xrd_shake_compensator_record (self->compensator, &intersection_pixels);

      gboolean is_drag = xrd_shake_compensator_is_drag (self->compensator,
                                                        self->hover_window,
                                                        controller_pose,
                                                        intersection);

      /* If we don't know yet, move cursor in VR pretending to be responsive.
       * If we predict drag, replay queue which contains "start of the drag".
       * If we predict click, queue only contains "shake" which we discard. */
       if (is_drag)
        {
          xrd_shake_compensator_replay_move_queue (
              self->compensator, self,
              signals[MOVE_CURSOR_EVENT], self->hover_window);
          xrd_shake_compensator_reset (self->compensator);
        }
      else
        {
          event->ignore = TRUE;
        }
    }

  g_signal_emit (self, signals[MOVE_CURSOR_EVENT], 0, event);
}

static void
_update_scroll_threshold (GSettings *settings, gchar *key, gpointer _self)
{
  XrdInputSynth *self = _self;
  self->scroll_threshold = (float) g_settings_get_double (settings, key);
}

static void
_update_shake_compensation_enabled (GSettings *settings,
                                    gchar *key,
                                    XrdInputSynth *self)
{
  self->compensator_enabled = g_settings_get_boolean (settings, key);
  if (!self->compensator_enabled)
    xrd_shake_compensator_reset (self->compensator);
}

/**
 * xrd_input_synth_create_action_set:
 * @self: the #XrdInputSynth
 * @context: A #GxrContext
 *
 * Returns: an ActionSet that will be centrally owned by, updated in and
 * destroyed in #XrdClient.
 */
GxrActionSet *
xrd_input_synth_create_action_set (XrdInputSynth *self, GxrContext *context)
{
  GxrActionSet *set =
    gxr_action_set_new_from_url (context, "/actions/mouse_synth");

  gxr_action_set_connect (set, context, GXR_ACTION_DIGITAL,
                          "/actions/mouse_synth/in/left_click",
                          (GCallback) _action_left_click_cb, self);
  gxr_action_set_connect (set, context, GXR_ACTION_DIGITAL,
                          "/actions/mouse_synth/in/right_click",
                          (GCallback) _action_right_click_cb, self);
  gxr_action_set_connect (set, context, GXR_ACTION_DIGITAL,
                          "/actions/mouse_synth/in/middle_click",
                          (GCallback) _action_middle_click_cb, self);
  gxr_action_set_connect (set, context, GXR_ACTION_VEC2F,
                          "/actions/mouse_synth/in/scroll",
                          (GCallback) _action_scroll_cb, self);

  xrd_settings_connect_and_apply (G_CALLBACK (_update_scroll_threshold),
                                  "scroll-threshold", self);

  xrd_settings_connect_and_apply
      (G_CALLBACK (_update_shake_compensation_enabled),
       "shake-compensation-enabled",
       self);

  return set;
}

static void
xrd_input_synth_init (XrdInputSynth *self)
{
  self->button_press_state = 0;
  graphene_point_init (&self->hover_position, 0, 0);

  self->compensator = xrd_shake_compensator_new ();
  self->primary_controller = 0;
}

/**
 * xrd_input_synth_get_primary_controller:
 * @self: The #XrdInputSynth
 *
 * Returns: The GxrController that is used for input synth.
 */
GxrController *
xrd_input_synth_get_primary_controller (XrdInputSynth *self)
{
  return self->primary_controller;
}

/**
 * xrd_input_synth_make_primary:
 * @self: The #XrdInputSynth
 * @controller_handle: The index of the controller that will be used for input
 * synth.
 */
void
xrd_input_synth_make_primary (XrdInputSynth *self,
                              GxrController *controller)
{
  xrd_input_synth_reset_scroll (self);
  xrd_input_synth_reset_press_state (self);
  self->primary_controller = controller;
}

/**
 * xrd_input_synth_reset_scroll:
 * @self: The #XrdInputSynth
 *
 * Resets the internal state of the scrolling, so the in-flight scroll distance
 * on the touchpad is discarded.
 *
 * When calling this function, also consider xrd_input_synth_reset_press_state()
 */
void
xrd_input_synth_reset_scroll (XrdInputSynth *self)
{
  graphene_vec3_init (&self->scroll_accumulator, 0, 0, 0);
}
