/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-shake-compensator.h"
#include "graphene-ext.h"
#include "xrd-settings.h"

struct _XrdShakeCompensator
{
  GObject parent;

  gint64 last_press_time;
  InputSynthButton last_press_button;
  GQueue *queue;

  double threshold_percent;
  int button_press_duration;
};

G_DEFINE_TYPE (XrdShakeCompensator, xrd_shake_compensator, G_TYPE_OBJECT)

static void
xrd_shake_compensator_finalize (GObject *gobject);

static void
xrd_shake_compensator_class_init (XrdShakeCompensatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_shake_compensator_finalize;
}

static void
xrd_shake_compensator_init (XrdShakeCompensator *self)
{
  self->queue = NULL;
  xrd_shake_compensator_reset (self);
  xrd_settings_connect_and_apply (G_CALLBACK (xrd_settings_update_double_val),
                                  "shake-compensation-threshold",
                                  &self->threshold_percent);
  xrd_settings_connect_and_apply (G_CALLBACK (xrd_settings_update_int_val),
                                  "shake-compensation-duration-ms",
                                  &self->button_press_duration);
}

XrdShakeCompensator *
xrd_shake_compensator_new (void)
{
  return (XrdShakeCompensator*) g_object_new (XRD_TYPE_SHAKE_COMPENSATOR, 0);
}

static void
xrd_shake_compensator_finalize (GObject *gobject)
{
  XrdShakeCompensator *self = XRD_SHAKE_COMPENSATOR (gobject);
  xrd_shake_compensator_reset (self);
}

void
xrd_shake_compensator_replay_move_queue (XrdShakeCompensator *self,
                                         XrdInputSynth *synth,
                                         guint move_cursor_event_signal,
                                         XrdWindow *hover_window)
{
  for (graphene_point_t *p = g_queue_pop_tail (self->queue); p != NULL;
       p = g_queue_pop_tail (self->queue))
    {
      // g_print ("Replay %f,%f\n", p->x, p->y);
      XrdMoveCursorEvent *replay  =  g_malloc (sizeof (XrdMoveCursorEvent));
      replay->window = hover_window;
      replay->position = p;
      replay->ignore = FALSE;
      g_signal_emit (synth, move_cursor_event_signal, 0, replay);

      g_free (p);

      /* we must not replay mouse move events too fast
       * or they will be dropped */
      g_usleep ((gulong)((1. / 10.) * G_USEC_PER_SEC / 1000.));
    }
  self->queue = NULL;
}

gboolean
xrd_shake_compensator_is_drag (XrdShakeCompensator *self,
                               XrdWindow *window,
                               graphene_matrix_t *controller_pose,
                               graphene_point3d_t *intersection)
{
  gint64 now = g_get_monotonic_time ();
  double ms_since_press = (double) (now - self->last_press_time) / 1000.;

  /* we'll only be able to predict after a few movements*/
  guint len = g_queue_get_length (self->queue);
  if (len  < 2)
    return FALSE;

  /* if longer than a usual click, it will be a drag */
  if ((int) ms_since_press > self->button_press_duration)
    return TRUE;


  graphene_point3d_t controller_point;
  graphene_ext_matrix_get_translation_point3d (controller_pose,
                                              &controller_point);

  float window_controller_dist = graphene_point3d_distance (intersection,
                                                            &controller_point,
                                                            NULL);

  float variance_meter = 0;

  graphene_point_t *start = g_queue_peek_tail (self->queue);

  /* If we only base the prediction on variance, we can look at only the last
   * mouse move since prediction happens on every mouse move.
   */
  graphene_point_t *p = g_queue_peek_head (self->queue);

  float dist_pixel = graphene_point_distance (start, p, 0, 0);

  float dist_meter = dist_pixel / xrd_window_get_current_ppm (window);

  variance_meter = fmaxf (dist_meter, variance_meter);

  /* Shaking should scale linearly with controller-window distance. */
  float variance_percent = (variance_meter / window_controller_dist) * 100.f;

  if (variance_percent > (float) self->threshold_percent)
    return TRUE;

  return FALSE;
}

void
xrd_shake_compensator_start_recording (XrdShakeCompensator *self,
                                       InputSynthButton button)
{
  if (!self->queue)
    self->queue = g_queue_new ();
  self->last_press_button = button;
  self->last_press_time = g_get_monotonic_time ();
}

void
xrd_shake_compensator_reset (XrdShakeCompensator *self)
{
  if (self->queue)
    {
      for (graphene_point_t *p = g_queue_pop_tail (self->queue); p != NULL;
           p = g_queue_pop_tail (self->queue))
        g_free (p);
    }
  self->queue = NULL;

  self->last_press_button = 0;
  self->last_press_time = 0;
}

InputSynthButton
xrd_shake_compensator_get_button (XrdShakeCompensator *self)
{
  return self->last_press_button;
}

gboolean
xrd_shake_compensator_is_recording (XrdShakeCompensator *self)
{
  return self->last_press_button != 0;
}

void
xrd_shake_compensator_record (XrdShakeCompensator *self,
                              graphene_point_t *position)
{
  graphene_point_t *p = g_malloc (sizeof (graphene_point_t));
  graphene_point_init_from_point (p, position);
  g_queue_push_head (self->queue, p);
}
