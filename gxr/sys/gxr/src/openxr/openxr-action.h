/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_ACTION_H_
#define GXR_OPENXR_ACTION_H_

#include <glib-object.h>
#include <graphene.h>
#include <openxr/openxr.h>

#include "gxr-action.h"
#include "openxr-context.h"
#include "openxr-action-set.h"

G_BEGIN_DECLS

#define OPENXR_TYPE_ACTION openxr_action_get_type()
G_DECLARE_FINAL_TYPE (OpenXRAction, openxr_action, OPENXR, ACTION, GxrAction)
OpenXRAction *
openxr_action_new_from_type_url (OpenXRContext *context,
                                 GxrActionSet *action_set,
                                 GxrActionType type, char *url);

OpenXRAction *openxr_action_new (OpenXRContext *context);

void
openxr_action_update_controllers (OpenXRAction *self);

uint32_t
openxr_action_get_num_bindings (OpenXRAction *self);

void
openxr_action_set_bindings (OpenXRAction *self,
                            XrActionSuggestedBinding *bindings);

char *
openxr_action_get_url (OpenXRAction *self);

XrAction
openxr_action_get_handle (OpenXRAction *self);

G_END_DECLS

#endif /* GXR_OPENXR_ACTION_H_ */
