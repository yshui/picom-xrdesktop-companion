/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_ACTION_H_
#define GXR_ACTION_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>
#include <graphene.h>

#include "gxr-enums.h"
#include "gxr-types.h"

G_BEGIN_DECLS

typedef struct _GxrContext GxrContext;

#define GXR_TYPE_ACTION gxr_action_get_type()
G_DECLARE_DERIVABLE_TYPE (GxrAction, gxr_action, GXR, ACTION, GObject)

typedef struct _GxrActionSet GxrActionSet;

struct _GxrActionClass
{
  GObjectClass parent;

  gboolean
  (*poll) (GxrAction *action);

  gboolean
  (*trigger_haptic) (GxrAction *action,
                     float start_seconds_from_now,
                     float duration_seconds,
                     float frequency,
                     float amplitude,
                     guint64 controller_handle);

  void
  (*set_digital_from_float_threshold) (GxrAction *self,
                                       float      threshold);

  void
  (*set_digital_from_float_haptic) (GxrAction *self,
                                    GxrAction *haptic_action);
};

GxrAction *
gxr_action_new_from_type_url (GxrContext   *context,
                              GxrActionSet *action_set,
                              GxrActionType type,
                              char          *url);

gboolean
gxr_action_poll (GxrAction *self);

gboolean
gxr_action_trigger_haptic (GxrAction *self,
                           float start_seconds_from_now,
                           float duration_seconds,
                           float frequency,
                           float amplitude,
                           guint64 controller_handle);

GxrActionType
gxr_action_get_action_type (GxrAction *self);

GxrActionSet *
gxr_action_get_action_set (GxrAction *self);

gchar*
gxr_action_get_url (GxrAction *self);

void
gxr_action_set_action_type (GxrAction *self, GxrActionType type);

void
gxr_action_set_action_set (GxrAction *self, GxrActionSet *action_set);

void
gxr_action_set_url (GxrAction *self, gchar* url);

void
gxr_action_emit_digital (GxrAction       *self,
                         GxrDigitalEvent *event);

void
gxr_action_emit_analog (GxrAction      *self,
                        GxrAnalogEvent *event);

void
gxr_action_emit_pose (GxrAction    *self,
                      GxrPoseEvent *event);

void
gxr_action_set_digital_from_float_threshold (GxrAction *self,
                                             float      threshold);

void
gxr_action_set_digital_from_float_haptic (GxrAction *self,
                                          GxrAction *haptic_action);

G_END_DECLS

#endif /* GXR_ACTION_H_ */
