/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_ENUMS_H_
#define XRD_ENUMS_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

typedef enum {
  XRD_CLIENT_MODE_OVERLAY,
  XRD_CLIENT_MODE_SCENE
} XrdClientMode;

#endif /* XRD_ENUMS_H_ */
