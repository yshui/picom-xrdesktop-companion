/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXRDEVICE_MANAGER_H_
#define GXRDEVICE_MANAGER_H_

#include <glib-object.h>

#include <gulkan.h>

#include "gxr-enums.h"
#include "gxr-types.h"

#include "gxr-device.h"
#include "gxr-action-set.h"

struct _GxrContext;

G_BEGIN_DECLS

#define GXR_TYPE_DEVICE_MANAGER gxr_device_manager_get_type()
G_DECLARE_FINAL_TYPE (GxrDeviceManager, gxr_device_manager,
                      GXR, DEVICE_MANAGER, GObject)

GxrDeviceManager *gxr_device_manager_new (void);

gboolean
gxr_device_manager_add (GxrDeviceManager   *self,
                        struct _GxrContext *context,
                        guint64             device_id,
                        bool                is_controller);

void
gxr_device_manager_remove (GxrDeviceManager *self,
                           guint64           device_id);

void
gxr_device_manager_update_poses (GxrDeviceManager *self, GxrPose *poses);

GSList *
gxr_device_manager_get_controllers (GxrDeviceManager *self);

GxrDevice *
gxr_device_manager_get (GxrDeviceManager *self, guint64 device_id);

GList *
gxr_device_manager_get_devices (GxrDeviceManager *self);

void
gxr_device_manager_connect_pose_actions (GxrDeviceManager   *self,
                                         struct _GxrContext *context,
                                         GxrActionSet       *action_set,
                                         gchar              *pointer_pose_url,
                                         gchar              *hand_grip_pose_url);

G_END_DECLS

#endif /* GXRDEVICE_MANAGER_H_ */
