/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-action.h"

#include <gdk/gdk.h>

#include "gxr-types.h"
#include "gxr-context.h"
#include "gxr-action-set.h"
#include "gxr-context-private.h"

typedef struct _GxrActionPrivate
{
  GObject parent;

  GxrActionSet *action_set;
  gchar *url;

  GxrActionType type;
} GxrActionPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrAction, gxr_action, G_TYPE_OBJECT)

enum {
  DIGITAL_EVENT,
  ANALOG_EVENT,
  POSE_EVENT,
  LAST_SIGNAL
};

static guint action_signals[LAST_SIGNAL] = { 0 };

static void
gxr_action_finalize (GObject *gobject);

gboolean
gxr_action_load_handle (GxrAction *self,
                        char      *url);

static void
gxr_action_class_init (GxrActionClass *klass)
{
  action_signals[DIGITAL_EVENT] =
    g_signal_new ("digital-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  action_signals[ANALOG_EVENT] =
    g_signal_new ("analog-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  action_signals[POSE_EVENT] =
    g_signal_new ("pose-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_action_finalize;
}

static void
gxr_action_init (GxrAction *self)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  priv->action_set = NULL;
  priv->url = NULL;
}

GxrAction *
gxr_action_new_from_type_url (GxrContext   *context,
                              GxrActionSet *action_set,
                              GxrActionType type,
                              char          *url)
{
  return gxr_context_new_action_from_type_url (context, action_set, type, url);
}

gboolean
gxr_action_poll (GxrAction *self)
{
  GxrActionClass *klass = GXR_ACTION_GET_CLASS (self);
  if (klass->poll == NULL)
      return FALSE;
  return klass->poll (self);
}

gboolean
gxr_action_trigger_haptic (GxrAction *self,
                           float start_seconds_from_now,
                           float duration_seconds,
                           float frequency,
                           float amplitude,
                           guint64 controller_handle)
{
  GxrActionClass *klass = GXR_ACTION_GET_CLASS (self);
  if (klass->trigger_haptic == NULL)
      return FALSE;
  return klass->trigger_haptic (self,
                                start_seconds_from_now,
                                duration_seconds,
                                frequency,
                                amplitude,
                                controller_handle);
}

static void
gxr_action_finalize (GObject *gobject)
{
  GxrAction *self = GXR_ACTION (gobject);
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  g_free (priv->url);
}

GxrActionType
gxr_action_get_action_type (GxrAction *self)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  return priv->type;
}

GxrActionSet *
gxr_action_get_action_set (GxrAction *self)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  return priv->action_set;
}

gchar*
gxr_action_get_url (GxrAction *self)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  return priv->url;
}

void
gxr_action_set_action_type (GxrAction *self, GxrActionType type)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  priv->type = type;
}

void
gxr_action_set_action_set (GxrAction *self, GxrActionSet *action_set)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  priv->action_set = action_set;
}

void
gxr_action_set_url (GxrAction *self, gchar* url)
{
  GxrActionPrivate *priv = gxr_action_get_instance_private (self);
  priv->url = url;
}

void
gxr_action_emit_digital (GxrAction       *self,
                         GxrDigitalEvent *event)
{
  g_signal_emit (self, action_signals[DIGITAL_EVENT], 0, event);
}

void
gxr_action_emit_analog (GxrAction      *self,
                        GxrAnalogEvent *event)
{
  g_signal_emit (self, action_signals[ANALOG_EVENT], 0, event);
}

void
gxr_action_emit_pose (GxrAction    *self,
                      GxrPoseEvent *event)
{
  g_signal_emit (self, action_signals[POSE_EVENT], 0, event);
}

void
gxr_action_set_digital_from_float_threshold (GxrAction *self,
                                             float       threshold)
{
  GxrActionClass *klass = GXR_ACTION_GET_CLASS (self);
  if (klass->set_digital_from_float_threshold == NULL)
    return;
  klass->set_digital_from_float_threshold (self, threshold);
}

void
gxr_action_set_digital_from_float_haptic (GxrAction *self,
                                          GxrAction *haptic_action)
{
  GxrActionClass *klass = GXR_ACTION_GET_CLASS (self);
  if (klass->set_digital_from_float_haptic == NULL)
    return;
  klass->set_digital_from_float_haptic (self, haptic_action);
}
