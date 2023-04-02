/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-overlay-pointer.h"

struct _XrdOverlayPointer
{
  GObject parent;

  graphene_matrix_t transformation;

  GxrPointerData data;
};

static void
xrd_overlay_pointer_pointer_interface_init (GxrPointerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (XrdOverlayPointer, xrd_overlay_pointer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_POINTER,
                                                xrd_overlay_pointer_pointer_interface_init))

static void
xrd_overlay_pointer_class_init (XrdOverlayPointerClass *klass)
{
  (void) klass;
}

static void
xrd_overlay_pointer_init (XrdOverlayPointer *self)
{
  gxr_pointer_init (GXR_POINTER (self));
}

XrdOverlayPointer *
xrd_overlay_pointer_new ()
{
  XrdOverlayPointer *self =
    (XrdOverlayPointer*) g_object_new (XRD_TYPE_OVERLAY_POINTER, 0);

  return self;
}

static GxrPointerData*
_get_data (GxrPointer *pointer)
{
  XrdOverlayPointer *self = XRD_OVERLAY_POINTER (pointer);
  return &self->data;
}

static void
_set_transformation (GxrPointer        *pointer,
                     graphene_matrix_t *matrix)
{
  XrdOverlayPointer *self = XRD_OVERLAY_POINTER (pointer);
  graphene_matrix_init_from_matrix (&self->transformation, matrix);
}

static void
_get_transformation (GxrPointer        *pointer,
                     graphene_matrix_t *matrix)
{
  XrdOverlayPointer *self = XRD_OVERLAY_POINTER (pointer);
  graphene_matrix_init_from_matrix (matrix, &self->transformation);
}

static void
_move (GxrPointer        *pointer,
       graphene_matrix_t *transform)
{
  XrdOverlayPointer *self = XRD_OVERLAY_POINTER (pointer);
  graphene_matrix_init_from_matrix (&self->transformation, transform);
}

static void
xrd_overlay_pointer_pointer_interface_init (GxrPointerInterface *iface)
{
  iface->get_data = _get_data;
  iface->set_transformation = _set_transformation;
  iface->get_transformation = _get_transformation;
  iface->move = _move;
}
