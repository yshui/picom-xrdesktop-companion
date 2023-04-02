/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_IO_H_
#define GXR_IO_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

gboolean
gxr_io_create_directory_if_needed (gchar *path);

gboolean
gxr_io_write_resource_to_file (const gchar *res_base_path,
                               gchar *cache_path,
                               const gchar *file_name,
                               GString *file_path);

GString*
gxr_io_get_cache_path (const gchar* dir_name);

#endif /* GXR_IO_H_ */
