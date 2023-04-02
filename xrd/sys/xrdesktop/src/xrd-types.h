/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_TYPES_H_
#define XRD_TYPES_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <graphene.h>
#include "gxr.h"

/**
 * XrdHoverEvent:
 * @point: The point in 3D world space.
 * @pose: A #graphene_matrix_t pose.
 * @distance: Distance from the controller.
 * @controller: The controller the event was captured on.
 *
 * An event that gets emitted when a controller hovers a window.
 **/
typedef struct {
  graphene_point3d_t point;
  graphene_matrix_t  pose;
  float              distance;
  GxrController     *controller;
} XrdHoverEvent;

/**
 * XrdGrabEvent:
 * @pose: A #graphene_matrix_t pose.
 * @controller: The controller the event was captured on.
 *
 * An event that gets emitted when a window get grabbed.
 **/
typedef struct {
  graphene_matrix_t  pose;
  GxrController     *controller;
} XrdGrabEvent;

#endif /* XRD_TYPES_H_ */
