/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-model.h"

G_DEFINE_INTERFACE (GxrModel, gxr_model, G_TYPE_OBJECT)

static void
gxr_model_default_init (GxrModelInterface *iface)
{
  (void) iface;
}

gchar *
gxr_model_get_name (GxrModel *self)
{
  GxrModelInterface* iface = GXR_MODEL_GET_IFACE (self);
  return iface->get_name (self);
}
