/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SCENE_CLIENT_H_
#define XRD_SCENE_CLIENT_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include "xrd-client.h"

G_BEGIN_DECLS

#define XRD_TYPE_SCENE_CLIENT xrd_scene_client_get_type ()
G_DECLARE_FINAL_TYPE (XrdSceneClient, xrd_scene_client,
                      XRD, SCENE_CLIENT, XrdClient)

void xrd_scene_client_render (XrdSceneClient *self);

G_END_DECLS

#endif /* XRD_SCENE_CLIENT_H_ */
