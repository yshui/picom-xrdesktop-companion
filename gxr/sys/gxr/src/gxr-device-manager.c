/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-device-manager.h"

#include <gxr.h>
#include "gxr-device.h"
#include "gxr-context.h"
#include "gxr-controller.h"

struct _GxrDeviceManager
{
  GObject parent;

  GHashTable *devices; // gint64 -> XrdSceneDevice

  // controllers are also put into a list for easy controller only iteration
  GSList *controllers;

  GRecMutex device_mutex;
};

G_DEFINE_TYPE (GxrDeviceManager, gxr_device_manager, G_TYPE_OBJECT)

enum {
  DEVICE_ACTIVATE_EVENT,
  DEVICE_DEACTIVATE_EVENT,
  LAST_SIGNAL
};

static guint dm_signals[LAST_SIGNAL] = { 0 };

static void
gxr_device_manager_finalize (GObject *gobject);

static void
gxr_device_manager_class_init (GxrDeviceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_device_manager_finalize;

  dm_signals[DEVICE_ACTIVATE_EVENT] =
    g_signal_new ("device-activate-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);

  dm_signals[DEVICE_DEACTIVATE_EVENT] =
    g_signal_new ("device-deactivate-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE,
                  1, G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
gxr_device_manager_init (GxrDeviceManager *self)
{
  self->devices = g_hash_table_new_full (g_int64_hash, g_int64_equal,
                                         g_free, g_object_unref);
  self->controllers = NULL;

  g_rec_mutex_init (&self->device_mutex);
}

GxrDeviceManager *
gxr_device_manager_new (void)
{
  return (GxrDeviceManager*) g_object_new (GXR_TYPE_DEVICE_MANAGER, 0);
}

static void
gxr_device_manager_finalize (GObject *gobject)
{
  GxrDeviceManager *self = GXR_DEVICE_MANAGER (gobject);
  g_slist_free (self->controllers);
  g_hash_table_unref (self->devices);
  g_rec_mutex_clear (&self->device_mutex);
}

static void
_insert_at_key (GHashTable *table, guint64 key, gpointer value)
{
  guint64 *keyp = g_new0 (guint64, 1);
  *keyp = key;
  g_hash_table_insert (table, keyp, value);
}

static void
gxr_device_manager_emit_device_activate (GxrDeviceManager *self, gpointer event)
{
  g_signal_emit (self, dm_signals[DEVICE_ACTIVATE_EVENT], 0, event);
}

static void
gxr_device_manager_emit_device_deactivate (GxrDeviceManager *self,
                                           gpointer          event)
{
  g_signal_emit (self, dm_signals[DEVICE_DEACTIVATE_EVENT], 0, event);
}

GSList *
gxr_device_manager_get_controllers (GxrDeviceManager *self)
{
  g_rec_mutex_lock (&self->device_mutex);

  GSList *controllers = self->controllers;

  g_rec_mutex_unlock (&self->device_mutex);

  return controllers;
}

gboolean
gxr_device_manager_add (GxrDeviceManager *self,
                        GxrContext       *context,
                        guint64           device_id,
                        bool              is_controller)
{
  g_rec_mutex_lock (&self->device_mutex);

  if (g_hash_table_lookup (self->devices, &device_id) != NULL)
    {
      g_debug ("Device %lu already added\n", device_id);
      g_rec_mutex_unlock (&self->device_mutex);
      return FALSE;
    }

  gchar *model_name = gxr_context_get_device_model_name (context,
                                                         (uint32_t) device_id);

  GxrDevice *device;
  if (is_controller)
    {
      device = GXR_DEVICE (gxr_controller_new (device_id, context, model_name));
      gxr_device_manager_emit_device_activate (self, device);
    }
  else
    {
      device = gxr_device_new (device_id, model_name);
    }

  g_debug ("Created device for %lu, model %s, is controller: %d\n",
           device_id, model_name, is_controller);

  g_free (model_name);

  if (is_controller)
    self->controllers = g_slist_append (self->controllers, device);

  _insert_at_key (self->devices, device_id, device);

  g_rec_mutex_unlock (&self->device_mutex);

  return TRUE;
}

void
gxr_device_manager_remove (GxrDeviceManager *self,
                           guint64           device_id)
{
  g_rec_mutex_lock (&self->device_mutex);

  GxrDevice *device = g_hash_table_lookup (self->devices, &device_id);
  if (!device)
    {
      g_print ("Device Manager: Returned nonexistent device\n");
      g_rec_mutex_unlock (&self->device_mutex);
      return;
    }

  /* g_hash_table_remove destroys device */
  gxr_device_manager_emit_device_deactivate (self, device);

  if (gxr_device_is_controller (device))
    self->controllers = g_slist_remove (self->controllers, device);

  g_hash_table_remove (self->devices, &device_id);
  g_debug ("Destroyed device %lu\n", device_id);

  g_rec_mutex_unlock (&self->device_mutex);
}

void
gxr_device_manager_update_poses (GxrDeviceManager *self, GxrPose *poses)
{
  g_rec_mutex_lock (&self->device_mutex);

  GList *device_keys = g_hash_table_get_keys (self->devices);
  for (GList *l = device_keys; l; l = l->next)
    {
      guint64 *key = l->data;
      guint64 i = *key;

      GxrDevice *device = g_hash_table_lookup (self->devices, &i);

      if (!device)
        {
          g_print ("Device Manager: Device %lu not known?!\n", i);
          continue;
        }

      gxr_device_set_is_pose_valid (device, poses[i].is_valid);

      if (!poses[i].is_valid)
        continue;

      gxr_device_set_transformation_direct (device, &poses[i].transformation);
    }

  g_list_free (device_keys);

  g_rec_mutex_unlock (&self->device_mutex);
}

GxrDevice *
gxr_device_manager_get (GxrDeviceManager *self, guint64 device_id)
{
  g_rec_mutex_lock (&self->device_mutex);

  GxrDevice *d = g_hash_table_lookup (self->devices, &device_id);

  g_rec_mutex_unlock (&self->device_mutex);

  return d;
}

GList *
gxr_device_manager_get_devices (GxrDeviceManager *self)
{
  g_rec_mutex_lock (&self->device_mutex);

  GList *devices = g_hash_table_get_values (self->devices);

  g_rec_mutex_unlock (&self->device_mutex);

  return devices;
}

static void
_update_pointer_pose_cb (GxrAction        *action,
                         GxrPoseEvent     *event,
                         GxrDeviceManager *self)
{
  (void) action;
  (void) self;

  g_rec_mutex_lock (&self->device_mutex);

  gboolean valid = event->device_connected && event->active && event->valid;
  gxr_controller_update_pointer_pose (event->controller, &event->pose, valid);

  g_rec_mutex_unlock (&self->device_mutex);

  g_free (event);
}

static void
_update_hand_grip_pose_cb (GxrAction        *action,
                          GxrPoseEvent     *event,
                          GxrDeviceManager *self)
{
  (void) action;
  (void) self;

  g_rec_mutex_lock (&self->device_mutex);

  gboolean valid = event->device_connected && event->active && event->valid;
  gxr_controller_update_hand_grip_pose (event->controller, &event->pose, valid);

  g_rec_mutex_unlock (&self->device_mutex);

  g_free (event);
}

void
gxr_device_manager_connect_pose_actions (GxrDeviceManager *self,
                                         GxrContext       *context,
                                         GxrActionSet     *action_set,
                                         gchar            *pointer_pose_url,
                                         gchar            *hand_grip_pose_url)
{
  if (pointer_pose_url)
    gxr_action_set_connect (action_set, context, GXR_ACTION_POSE,
                            pointer_pose_url,
                            (GCallback) _update_pointer_pose_cb, self);

  if (hand_grip_pose_url)
    gxr_action_set_connect (action_set, context, GXR_ACTION_POSE,
                            hand_grip_pose_url,
                            (GCallback) _update_hand_grip_pose_cb, self);
}
