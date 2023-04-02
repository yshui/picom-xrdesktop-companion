/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_BACKEND_PRIVATE_H_
#define GXR_BACKEND_PRIVATE_H_

#include <glib-object.h>

#include "gxr-backend.h"
#include "gxr-enums.h"

G_BEGIN_DECLS

typedef struct _GxrContext GxrContext;

GxrBackend *
gxr_backend_get_instance (GxrApi api);

GxrContext *
gxr_backend_new_context (GxrBackend *self);

G_END_DECLS

#endif /* GXR_BACKEND_PRIVATE_H_ */
