/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-backend-private.h"

#include <gmodule.h>

#include "gxr-config.h"
#include "gxr-context-private.h"
#include "gxr-enums.h"
#include "openvr/openvr-context.h"
#include "openxr/openxr-context.h"

struct _GxrBackend
{
  GObject parent;
  GxrApi  api;
  GxrContext *(*context_new) (void);
};

G_DEFINE_TYPE (GxrBackend, gxr_backend, G_TYPE_OBJECT)

/* Backend singleton */
static GxrBackend *backend = NULL;

static void gxr_backend_finalize (GObject *gobject);

static void
gxr_backend_class_init (GxrBackendClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gxr_backend_finalize;
}

static GxrBackend *
_new_from_api (GxrApi api)
{
  GxrBackend *self = (GxrBackend *)g_object_new (GXR_TYPE_BACKEND, 0);
  self->api = api;

  switch (api)
    {
    case GXR_API_OPENVR:
      self->context_new = (GxrContext * (*)(void)) openvr_context_new;
      break;
    case GXR_API_OPENXR:
      self->context_new = (GxrContext * (*)(void)) openxr_context_new;
      break;
    default:
      g_printerr ("Invalid API provided.");
      return NULL;
    }

  return self;
}

GxrBackend *
gxr_backend_get_instance (GxrApi api)
{
  if (backend == NULL)
    backend = _new_from_api (api);
  return backend;
}

void
gxr_backend_shutdown (void)
{
  g_object_unref (backend);
}

static void
gxr_backend_init (GxrBackend *self)
{
  (void)self;
}

GxrContext *
gxr_backend_new_context (GxrBackend *self)
{
  GxrContext *context = self->context_new ();
  gxr_context_set_api (context, self->api);
  return context;
}

static void
gxr_backend_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gxr_backend_parent_class)->finalize (gobject);
}
