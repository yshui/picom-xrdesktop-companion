/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-action-set.h"
#include "gxr-action.h"
#include "gxr-context-private.h"

typedef struct _GxrActionSetPrivate
{
  GObject parent;

  GSList *actions;
} GxrActionSetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GxrActionSet, gxr_action_set, G_TYPE_OBJECT)

static void
gxr_action_set_finalize (GObject *gobject);

static void
gxr_action_set_class_init (GxrActionSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_action_set_finalize;
}

static void
gxr_action_set_init (GxrActionSet *self)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);
  priv->actions = NULL;
}

GxrActionSet *
gxr_action_set_new_from_url (GxrContext *context, gchar *url)
{
  return gxr_context_new_action_set_from_url (context, url);
}

static void
gxr_action_set_finalize (GObject *gobject)
{
  GxrActionSet *self = GXR_ACTION_SET (gobject);
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);

  g_slist_free (priv->actions);
}

gboolean
gxr_action_sets_poll (GxrActionSet **sets, uint32_t count)
{
  GxrActionSetClass *klass = GXR_ACTION_SET_GET_CLASS (sets[0]);
  if (klass->update == NULL)
      return FALSE;

  if (!klass->update (sets, count))
    return FALSE;

  for (uint32_t i = 0; i < count; i++)
    {
      GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (sets[i]);
      for (GSList *l = priv->actions; l != NULL; l = l->next)
        {
          GxrAction *action = (GxrAction*) l->data;

          /* haptic has no inputs, can't be polled */
          if (gxr_action_get_action_type (action) == GXR_ACTION_HAPTIC)
            continue;

          if (!gxr_action_poll (action))
            return FALSE;
        }
    }
  return TRUE;
}

GxrAction *
gxr_action_set_connect_digital_from_float (GxrActionSet *self,
                                           GxrContext   *context,
                                           gchar        *url,
                                           float         threshold,
                                           char         *haptic_url,
                                           GCallback     callback,
                                           gpointer      data)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);

  GxrActionSetClass *klass = GXR_ACTION_SET_GET_CLASS (self);
  if (klass->create_action == NULL)
    return FALSE;
  GxrAction *action = klass->create_action (self,
                                            context,
                                            GXR_ACTION_DIGITAL_FROM_FLOAT,
                                            url);

  if (action != NULL)
    priv->actions = g_slist_append (priv->actions, action);


  GxrAction *haptic_action = NULL;
  if (haptic_url)
    {
      haptic_action = gxr_action_new_from_type_url (context,
                                                    self,
                                                    GXR_ACTION_HAPTIC,
                                                    haptic_url);
      if (haptic_action != NULL)
        {
          priv->actions = g_slist_append (priv->actions, haptic_action);
          gxr_action_set_digital_from_float_haptic (action, haptic_action);
        }
    }
  gxr_action_set_digital_from_float_threshold (action, threshold);



  g_signal_connect (action, "digital-event", callback, data);

  return action;
}

gboolean
gxr_action_set_connect (GxrActionSet *self,
                        GxrContext   *context,
                        GxrActionType type,
                        gchar        *url,
                        GCallback     callback,
                        gpointer      data)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);

  GxrActionSetClass *klass = GXR_ACTION_SET_GET_CLASS (self);
  if (klass->create_action == NULL)
      return FALSE;
  GxrAction *action = klass->create_action (self, context, type, url);

  if (action != NULL)
    priv->actions = g_slist_append (priv->actions, action);
  else
    {
      g_printerr ("Failed to create/connect action %s\n", url);
      return FALSE;
    }

  switch (type)
    {
    case GXR_ACTION_DIGITAL:
      g_signal_connect (action, "digital-event", callback, data);
      break;
    case GXR_ACTION_FLOAT:
    case GXR_ACTION_VEC2F:
      g_signal_connect (action, "analog-event", callback, data);
      break;
    case GXR_ACTION_POSE:
      g_signal_connect (action, "pose-event", callback, data);
      break;
    case GXR_ACTION_HAPTIC:
      /* no input, only output */
      break;
    default:
      g_printerr ("Unknown action type %d\n", type);
      return FALSE;
    }

  return TRUE;
}


GSList *
gxr_action_set_get_actions (GxrActionSet *self)
{
  GxrActionSetPrivate *priv = gxr_action_set_get_instance_private (self);
  return priv->actions;
}

gboolean
gxr_action_sets_attach_bindings (GxrActionSet **sets,
                                 GxrContext *context,
                                 uint32_t count)
{
  GxrActionSetClass *klass = GXR_ACTION_SET_GET_CLASS (sets[0]);
  if (klass->attach_bindings == NULL)
    return TRUE; /* noop succeeds when no implementation  */
  return klass->attach_bindings (sets, context, count);
}
