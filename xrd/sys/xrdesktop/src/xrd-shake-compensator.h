/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SHAKE_COMPENSATOR_H_
#define XRD_SHAKE_COMPENSATOR_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>
#include "xrd-window.h"
#include "xrd-input-synth.h"

G_BEGIN_DECLS

#define XRD_TYPE_SHAKE_COMPENSATOR xrd_shake_compensator_get_type()
G_DECLARE_FINAL_TYPE (XrdShakeCompensator, xrd_shake_compensator, XRD, SHAKE_COMPENSATOR, GObject)

XrdShakeCompensator *xrd_shake_compensator_new (void);

void
xrd_shake_compensator_replay_move_queue (XrdShakeCompensator *self,
                                         XrdInputSynth *synth,
                                         guint move_cursor_event_signal,
                                         XrdWindow *hover_window);

gboolean
xrd_shake_compensator_is_drag (XrdShakeCompensator *self,
                               XrdWindow *window,
                               graphene_matrix_t *controller_pose,
                               graphene_point3d_t *intersection);

void
xrd_shake_compensator_start_recording (XrdShakeCompensator *self,
                                       InputSynthButton button);

void
xrd_shake_compensator_reset (XrdShakeCompensator *self);

InputSynthButton
xrd_shake_compensator_get_button (XrdShakeCompensator *self);

gboolean
xrd_shake_compensator_is_recording (XrdShakeCompensator *self);

void
xrd_shake_compensator_record (XrdShakeCompensator *self,
                              graphene_point_t *position);

G_END_DECLS

#endif /* XRD_SHAKE_COMPENSATOR_H_ */
