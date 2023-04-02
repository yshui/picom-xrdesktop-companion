/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_MODEL_H_
#define GXR_MODEL_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

#include <gulkan.h>

G_BEGIN_DECLS

#define GXR_TYPE_MODEL gxr_model_get_type()
G_DECLARE_INTERFACE (GxrModel, gxr_model, GXR, MODEL, GObject)

struct _GxrModelInterface
{
  GTypeInterface parent;

  gchar *
  (*get_name) (GxrModel *self);
};

gchar *
gxr_model_get_name (GxrModel *self);

G_END_DECLS

#endif /* GXR_MODEL_H_ */
