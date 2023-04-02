/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_SETTINGS_H_
#define XRD_SETTINGS_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <gio/gio.h>

gboolean
xrd_settings_is_schema_installed (void);

GSettings *
xrd_settings_get_instance (void);

void
xrd_settings_destroy_instance (void);

void
xrd_settings_connect_and_apply (GCallback callback, gchar *key, gpointer data);

void
xrd_settings_update_double_val (GSettings *settings,
                                gchar *key,
                                double *val);
void
xrd_settings_update_int_val (GSettings *settings,
                             gchar *key,
                             int *val);
void
xrd_settings_update_gboolean_val (GSettings *settings,
                                  gchar *key,
                                  gboolean *val);

#endif /* XRD_SETTINGS_H_ */
