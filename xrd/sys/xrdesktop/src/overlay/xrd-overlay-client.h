/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_OVERLAY_CLIENT_H_
#define XRD_OVERLAY_CLIENT_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include "xrd-client.h"

G_BEGIN_DECLS

#define XRD_TYPE_OVERLAY_CLIENT xrd_overlay_client_get_type()
G_DECLARE_FINAL_TYPE (XrdOverlayClient, xrd_overlay_client,
                      XRD, OVERLAY_CLIENT, XrdClient)

G_END_DECLS

#endif /* XRD_OVERLAY_CLIENT_H_ */
