/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib/gprintf.h>

#include "gxr-types.h"

#include "openvr-wrapper.h"

#include "openvr-compositor.h"
#include "openvr-context.h"
#include "openvr-context.h"
#include "openvr-math.h"

#define ENUM_TO_STR(r) case r: return #r

static const gchar*
_compositor_error_to_str (EVRCompositorError err)
{
  switch (err)
    {
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_None);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_RequestFailed);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_IncompatibleVersion);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_DoNotHaveFocus);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_InvalidTexture);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_IsNotSceneApplication);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_TextureIsOnWrongDevice);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_TextureUsesUnsupportedFormat);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_SharedTexturesNotSupported);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_IndexOutOfRange);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_AlreadySubmitted);
      ENUM_TO_STR(EVRCompositorError_VRCompositorError_InvalidBounds);
      default:
        return "Unknown compositor error.";
    }
}

gboolean
openvr_compositor_submit (GulkanClient         *client,
                          uint32_t              width,
                          uint32_t              height,
                          VkFormat              format,
                          VkSampleCountFlagBits sample_count,
                          VkImage               left,
                          VkImage               right)
{
  GulkanDevice *device = gulkan_client_get_device (client);
  VkDevice device_handle = gulkan_device_get_handle (device);

  GulkanQueue *queue = gulkan_device_get_graphics_queue (device);

  VRVulkanTextureData_t openvr_texture_data = {
    .m_nImage = (uint64_t) left,
    .m_pDevice = device_handle,
    .m_pPhysicalDevice = gulkan_client_get_physical_device_handle (client),
    .m_pInstance = gulkan_client_get_instance_handle (client),
    .m_pQueue = gulkan_queue_get_handle (queue),
    .m_nQueueFamilyIndex = gulkan_queue_get_family_index (queue),
    .m_nWidth = width,
    .m_nHeight = height,
    .m_nFormat = format,
    .m_nSampleCount = sample_count
  };

  Texture_t texture = {
    &openvr_texture_data,
    ETextureType_TextureType_Vulkan,
    EColorSpace_ColorSpace_Auto
  };

  VRTextureBounds_t bounds = {
    .uMin = 0.0f,
    .uMax = 1.0f,
    .vMin = 0.0f,
    .vMax = 1.0f
  };

  OpenVRFunctions *f = openvr_get_functions ();

  EVRCompositorError err =
    f->compositor->Submit (EVREye_Eye_Left, &texture, &bounds,
                           EVRSubmitFlags_Submit_Default);

  if (err != EVRCompositorError_VRCompositorError_None)
    {
      g_printerr ("Submit returned error: %s\n",
                  _compositor_error_to_str (err));
      return FALSE;
    }

  openvr_texture_data.m_nImage = (uint64_t) right;
  err = f->compositor->Submit (EVREye_Eye_Right, &texture, &bounds,
                               EVRSubmitFlags_Submit_Default);

  if (err != EVRCompositorError_VRCompositorError_None)
    {
      g_printerr ("Submit returned error: %s\n",
                  _compositor_error_to_str (err));
      return FALSE;
    }

  return TRUE;
}

void
openvr_compositor_wait_get_poses (GxrPose *poses, uint32_t count)
{
  OpenVRFunctions *f = openvr_get_functions ();

  struct TrackedDevicePose_t *p =
    g_malloc (sizeof (struct TrackedDevicePose_t) * count);

  f->compositor->WaitGetPoses (p, count, NULL, 0);

  for (uint32_t i = 0; i < count; i++)
    {
      poses[i].is_valid = p[i].bPoseIsValid;
      if (poses[i].is_valid)
        openvr_math_matrix34_to_graphene (&p[i].mDeviceToAbsoluteTracking,
                                          &poses[i].transformation);
    }

  g_free (p);
}
