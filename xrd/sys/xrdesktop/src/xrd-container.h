/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_CONTAINER_H_
#define XRD_CONTAINER_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>
#include <gxr.h>

#include "xrd-window.h"

G_BEGIN_DECLS

#define XRD_TYPE_CONTAINER xrd_container_get_type()
G_DECLARE_FINAL_TYPE (XrdContainer, xrd_container, XRD, CONTAINER, GObject)

/**
 * XrdContainerAttachment:
 * @XRD_CONTAINER_ATTACHMENT_NONE: The #XrdContainer is not attached.
 * @XRD_CONTAINER_ATTACHMENT_HEAD: The container is tracking the head.
 * @XRD_CONTAINER_ATTACHMENT_HAND: The container is tracking a hand.
 *
 * Enum that defines if the container is moving with user input.
 *
 **/
typedef enum {
  XRD_CONTAINER_ATTACHMENT_NONE,
  XRD_CONTAINER_ATTACHMENT_HEAD,
  XRD_CONTAINER_ATTACHMENT_HAND
} XrdContainerAttachment;

/**
 * XrdContainerLayout:
 * @XRD_CONTAINER_NO_LAYOUT: No layout is set.
 * @XRD_CONTAINER_HORIZONTAL: A horizontal linear layout.
 * @XRD_CONTAINER_VERTICAL: A vertical linear layout.
 * @XRD_CONTAINER_RELATIVE: A relative layout.
 *
 * Defines how the children of a #XrdContainer are layed out.
 *
 **/
typedef enum {
  XRD_CONTAINER_NO_LAYOUT,
  XRD_CONTAINER_HORIZONTAL,
  XRD_CONTAINER_VERTICAL,
  XRD_CONTAINER_RELATIVE
} XrdContainerLayout;

struct _XrdContainer;

XrdContainer*
xrd_container_new (void);

void
xrd_container_add_window (XrdContainer *self,
                          XrdWindow *window,
                          graphene_matrix_t *relative_transform);

void
xrd_container_remove_window (XrdContainer *self,
                             XrdWindow *window);

void
xrd_container_set_distance (XrdContainer *self, float distance);

float
xrd_container_get_distance (XrdContainer *self);

void
xrd_container_set_attachment (XrdContainer *self,
                              XrdContainerAttachment attachment,
                              GxrController *controller);

void
xrd_container_set_layout (XrdContainer *self,
                          XrdContainerLayout layout);

gboolean
xrd_container_step (XrdContainer *self, GxrContext *context);

void
xrd_container_hide (XrdContainer *self);

void
xrd_container_show (XrdContainer *self);

gboolean
xrd_container_is_visible (XrdContainer *self);

GSList *
xrd_container_get_windows (XrdContainer *self);

void
xrd_container_center_view (XrdContainer *self, GxrContext *context,
                           float distance);

G_END_DECLS

#endif /* xrd_container_H_ */
