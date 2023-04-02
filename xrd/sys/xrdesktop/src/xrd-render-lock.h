/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Chrtstoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-2.0/glib.h>

#ifndef XRD_RENDER_LOCK_H_
#define XRD_RENDER_LOCK_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

void
xrd_render_lock_init (void);

void
xrd_render_lock_destroy (void);

void
xrd_render_lock (void);

void
xrd_render_unlock (void);

#endif // XRD_RENDER_LOCK_H_
