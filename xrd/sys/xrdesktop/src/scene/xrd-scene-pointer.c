/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-scene-pointer.h"
#include "xrd-render-lock.h"

#include <gulkan.h>

#include "graphene-ext.h"
#include "gxr-pointer.h"

typedef struct __attribute__((__packed__)) {
  float mvp[16];
} XrdScenePointerUniformBuffer;

static void
xrd_scene_pointer_interface_init (GxrPointerInterface *iface);

struct _XrdScenePointer
{
  XrdSceneObject parent;
  GulkanVertexBuffer *vertex_buffer;

  GxrPointerData data;
};

G_DEFINE_TYPE_WITH_CODE (XrdScenePointer, xrd_scene_pointer, XRD_TYPE_SCENE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GXR_TYPE_POINTER,
                                                xrd_scene_pointer_interface_init))

static void
xrd_scene_pointer_finalize (GObject *gobject);

static void
xrd_scene_pointer_class_init (XrdScenePointerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xrd_scene_pointer_finalize;
}

static void
xrd_scene_pointer_init (XrdScenePointer *self)
{
  self->vertex_buffer = gulkan_vertex_buffer_new ();

  gxr_pointer_init (GXR_POINTER (self));
}

static gboolean
_initialize (XrdScenePointer       *self,
             GulkanClient          *gulkan,
             VkDescriptorSetLayout *layout)
{
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->data.start_offset, 1);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  gulkan_geometry_append_ray (self->vertex_buffer,
                              &start, self->data.length, &identity);

  GulkanDevice *device = gulkan_client_get_device (gulkan);

  if (!gulkan_vertex_buffer_alloc_empty (self->vertex_buffer, device,
    GXR_DEVICE_INDEX_MAX))
    return FALSE;

  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);

  VkDeviceSize ubo_size = sizeof (XrdScenePointerUniformBuffer);

  if (!xrd_scene_object_initialize (obj, gulkan, layout, ubo_size))
    return FALSE;

  xrd_scene_object_update_descriptors (obj);

  return TRUE;
}

XrdScenePointer *
xrd_scene_pointer_new (GulkanClient          *gulkan,
                       VkDescriptorSetLayout *layout)
{
  XrdScenePointer *self =
    (XrdScenePointer*) g_object_new (XRD_TYPE_SCENE_POINTER, 0);

  _initialize (self, gulkan, layout);
  return self;
}

static void
xrd_scene_pointer_finalize (GObject *gobject)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (gobject);
  g_clear_object (&self->vertex_buffer);
  G_OBJECT_CLASS (xrd_scene_pointer_parent_class)->finalize (gobject);
}

static void
_update_ubo (XrdScenePointer   *self,
             GxrEye             eye,
             graphene_matrix_t *vp)
{
  XrdScenePointerUniformBuffer ub;

  graphene_matrix_t m_matrix;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&m_matrix, vp, &mvp_matrix);

  float mvp[16];
  graphene_matrix_to_float (&mvp_matrix, mvp);
  for (int i = 0; i < 16; i++)
    ub.mvp[i] = mvp[i];

  xrd_scene_object_update_ubo (XRD_SCENE_OBJECT (self), eye, &ub);
}

void
xrd_scene_pointer_render (XrdScenePointer   *self,
                          GxrEye             eye,
                          VkPipeline         pipeline,
                          VkPipelineLayout   pipeline_layout,
                          VkCommandBuffer    cmd_buffer,
                          graphene_matrix_t *vp)
{
  xrd_render_lock ();

  if (!gulkan_vertex_buffer_is_initialized (self->vertex_buffer))
    {
      xrd_render_unlock ();
      return;
    }

  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  if (!xrd_scene_object_is_visible (obj))
    {
      xrd_render_unlock ();
      return;
    }

  _update_ubo (self, eye, vp);

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  xrd_scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);

  xrd_render_unlock ();
}

static void
_move (GxrPointer        *pointer,
       graphene_matrix_t *transform)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  XrdSceneObject *obj = XRD_SCENE_OBJECT (self);
  xrd_scene_object_set_transformation_direct (obj, transform);
}

static void
_set_length (GxrPointer *pointer,
             float       length)
{
  xrd_render_lock ();

  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  gulkan_vertex_buffer_reset (self->vertex_buffer);

  graphene_matrix_t identity;
  graphene_matrix_init_identity (&identity);

  graphene_vec4_t start;
  graphene_vec4_init (&start, 0, 0, self->data.start_offset, 1);

  gulkan_geometry_append_ray (self->vertex_buffer, &start, length, &identity);
  gulkan_vertex_buffer_map_array (self->vertex_buffer);

  xrd_render_unlock ();
}

static GxrPointerData*
_get_data (GxrPointer *pointer)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  return &self->data;
}

static void
_set_transformation (GxrPointer        *pointer,
                     graphene_matrix_t *matrix)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  xrd_scene_object_set_transformation (XRD_SCENE_OBJECT (self), matrix);
}

static void
_get_transformation (GxrPointer        *pointer,
                     graphene_matrix_t *matrix)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  graphene_matrix_t transformation;
  xrd_scene_object_get_transformation (XRD_SCENE_OBJECT (self),
                                       &transformation);
  graphene_matrix_init_from_matrix (matrix, &transformation);
}

static void
_show (GxrPointer *pointer)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  xrd_scene_object_show (XRD_SCENE_OBJECT (self));
}

static void
_hide (GxrPointer *pointer)
{
  XrdScenePointer *self = XRD_SCENE_POINTER (pointer);
  xrd_scene_object_hide (XRD_SCENE_OBJECT (self));
}

static void
xrd_scene_pointer_interface_init (GxrPointerInterface *iface)
{
  iface->move = _move;
  iface->set_length = _set_length;
  iface->get_data = _get_data;
  iface->set_transformation = _set_transformation;
  iface->get_transformation = _get_transformation;
  iface->show = _show;
  iface->hide = _hide;
}

