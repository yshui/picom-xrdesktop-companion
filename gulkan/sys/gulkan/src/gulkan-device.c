/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gulkan-device.h"
#include "gulkan-queue.h"

struct _GulkanDevice
{
  GObjectClass parent_class;

  VkDevice device;
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties physical_props;

  VkPhysicalDeviceMemoryProperties memory_properties;

  GulkanQueue *graphics_queue;
  gboolean has_transfer_queue;
  GulkanQueue *transfer_queue;

  PFN_vkGetMemoryFdKHR extVkGetMemoryFdKHR;
};

G_DEFINE_TYPE (GulkanDevice, gulkan_device, G_TYPE_OBJECT)

static void
gulkan_device_init (GulkanDevice *self)
{
  self->device = VK_NULL_HANDLE;
  self->physical_device = VK_NULL_HANDLE;
  self->has_transfer_queue = FALSE;
  self->transfer_queue = NULL;
  self->graphics_queue = NULL;
  self->extVkGetMemoryFdKHR = 0;
}

GulkanDevice *
gulkan_device_new (void)
{
  return (GulkanDevice*) g_object_new (GULKAN_TYPE_DEVICE, 0);
}

static void
gulkan_device_finalize (GObject *gobject)
{
  GulkanDevice *self = GULKAN_DEVICE (gobject);
  if (self->has_transfer_queue)
    {
      if (self->transfer_queue) g_object_unref (self->transfer_queue);
    }
  if (self->graphics_queue) g_object_unref (self->graphics_queue);
  vkDestroyDevice (self->device, NULL);

  G_OBJECT_CLASS (gulkan_device_parent_class)->finalize (gobject);
}

static void
gulkan_device_class_init (GulkanDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gulkan_device_finalize;
}

static gboolean
_find_physical_device (GulkanDevice    *self,
                       GulkanInstance  *instance,
                       VkPhysicalDevice requested_device)
{
  VkInstance vk_instance = gulkan_instance_get_handle (instance);
  uint32_t num_devices = 0;
  VkResult res =
    vkEnumeratePhysicalDevices (vk_instance, &num_devices, NULL);
  vk_check_error ("vkEnumeratePhysicalDevices", res, FALSE);

  if (num_devices == 0)
    {
      g_printerr ("No Vulkan devices found.\n");
      return FALSE;
    }

  VkPhysicalDevice *physical_devices =
    g_malloc(sizeof(VkPhysicalDevice) * num_devices);
  res = vkEnumeratePhysicalDevices (vk_instance,
                                   &num_devices,
                                    physical_devices);
  vk_check_error ("vkEnumeratePhysicalDevices", res, FALSE);

  if (requested_device == VK_NULL_HANDLE)
    {
      /* No device requested. Using first one */
      self->physical_device = physical_devices[0];
    }
  else
    {
      /* Find requested device */
      self->physical_device = VK_NULL_HANDLE;
      for (uint32_t i = 0; i < num_devices; i++)
        if (physical_devices[i] == requested_device)
          {
            self->physical_device = requested_device;
            break;
          }

      if (self->physical_device == VK_NULL_HANDLE)
        {
          g_printerr ("Failed to find requested VkPhysicalDevice, "
                      "falling back to the first one.\n");
          self->physical_device = physical_devices[0];
        }
    }

  g_free (physical_devices);

  vkGetPhysicalDeviceMemoryProperties (self->physical_device,
                                      &self->memory_properties);

  return TRUE;
}

static gboolean
find_queue_index_for_flags (VkQueueFlagBits          flags,
                            VkQueueFlagBits          exclude_flags,
                            uint32_t                 num_queues,
                            VkQueueFamilyProperties *queue_family_props,
                            uint32_t                 *out_index)
{
  uint32_t i = 0;
  for (i = 0; i < num_queues; i++)
    if (queue_family_props[i].queueFlags & flags &&
        !(queue_family_props[i].queueFlags & exclude_flags))
      break;

  if (i >= num_queues)
    return FALSE;

  *out_index = i;
  return TRUE;
}

static gboolean
_find_queue_families (GulkanDevice *self)
{
  /* Find the first graphics queue */
  uint32_t num_queues = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (
    self->physical_device, &num_queues, 0);

  VkQueueFamilyProperties *queue_family_props =
    g_malloc (sizeof(VkQueueFamilyProperties) * num_queues);

  vkGetPhysicalDeviceQueueFamilyProperties (
    self->physical_device, &num_queues, queue_family_props);

  if (num_queues == 0)
    {
      g_printerr ("Failed to get queue properties.\n");
      return FALSE;
    }

  uint32_t graphics_queue_index = UINT32_MAX;
  if (!find_queue_index_for_flags (VK_QUEUE_GRAPHICS_BIT,
                                   (VkQueueFlagBits)0,
                                   num_queues,
                                   queue_family_props,
                                   &graphics_queue_index))
    {
      g_printerr ("No graphics queue found\n");
      return FALSE;
    }
  self->graphics_queue = gulkan_queue_new (self, graphics_queue_index);

  uint32_t transfer_queue_index = UINT32_MAX;
  if (find_queue_index_for_flags (VK_QUEUE_TRANSFER_BIT,
                                  VK_QUEUE_GRAPHICS_BIT,
                                  num_queues,
                                  queue_family_props,
                                  &transfer_queue_index))
    {
      g_debug("Got separate transfer queue");
      self->transfer_queue = gulkan_queue_new (self, transfer_queue_index);
      self->has_transfer_queue = TRUE;
    }
  else
    {
      g_debug ("No separate transfer queue found, using graphics queue");
    }

  g_free (queue_family_props);

  return TRUE;
}

static gboolean
_get_device_extension_count (GulkanDevice *self,
                             uint32_t     *count)
{
  VkResult res =
    vkEnumerateDeviceExtensionProperties (self->physical_device, NULL,
                                          count, NULL);
  vk_check_error ("vkEnumerateDeviceExtensionProperties", res, FALSE);
  return TRUE;
}

static gboolean
_init_device_extensions (GulkanDevice *self,
                         GSList       *required_extensions,
                         uint32_t      num_extensions,
                         char        **extension_names,
                         uint32_t     *out_num_enabled)
{
  uint32_t num_enabled = 0;

  /* Enable required device extensions */
  VkExtensionProperties *extension_props =
    g_malloc (sizeof(VkExtensionProperties) * num_extensions);

  memset (extension_props, 0, sizeof (VkExtensionProperties) * num_extensions);

  VkResult res = vkEnumerateDeviceExtensionProperties (self->physical_device,
                                                       NULL,
                                                      &num_extensions,
                                                       extension_props);
  vk_check_error ("vkEnumerateDeviceExtensionProperties", res, FALSE);
  for (uint32_t i = 0; i < g_slist_length (required_extensions); i++)
    {
      gboolean found = FALSE;
      for (uint32_t j = 0; j < num_extensions; j++)
        {
          GSList* extension_name = g_slist_nth (required_extensions, i);
          if (strcmp ((gchar*) extension_name->data,
                      extension_props[j].extensionName) == 0)
            {
              found = TRUE;
              break;
            }
        }

      if (found)
        {
          GSList* extension_name = g_slist_nth (required_extensions, i);
          extension_names[num_enabled] =
            g_strdup ((char*) extension_name->data);
          num_enabled++;
        }
    }

  *out_num_enabled = num_enabled;

  g_free (extension_props);

  return TRUE;
}

gboolean
gulkan_device_create (GulkanDevice    *self,
                      GulkanInstance  *instance,
                      VkPhysicalDevice device,
                      GSList          *extensions)
{
  if (!_find_physical_device (self, instance, device))
    return FALSE;

  vkGetPhysicalDeviceProperties (self->physical_device, &self->physical_props);

  if (!_find_queue_families (self))
    return FALSE;

  uint32_t num_extensions = 0;
  if (!_get_device_extension_count (self, &num_extensions))
    return FALSE;

  char **extension_names = g_malloc (sizeof (char*) * num_extensions);
  uint32_t num_enabled = 0;

  if (num_extensions > 0)
    {
      if (!_init_device_extensions (self, extensions, num_extensions,
                                    extension_names, &num_enabled))
        return FALSE;
    }

  if (num_enabled > 0)
    {
      g_debug ("Requesting device extensions:");
      for (uint32_t i = 0; i < num_enabled; i++)
        g_debug ("%s", extension_names[i]);
    }

  VkPhysicalDeviceFeatures physical_device_features;
  vkGetPhysicalDeviceFeatures (self->physical_device,
                              &physical_device_features);

  VkDeviceCreateInfo device_info =
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = self->has_transfer_queue ? 2 : 1,
      .pQueueCreateInfos = self->has_transfer_queue ?
          (VkDeviceQueueCreateInfo[]) {
            {
              .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
              .queueFamilyIndex = gulkan_queue_get_family_index(self->graphics_queue),
              .queueCount = 1,
              .pQueuePriorities = (const float[]) { 1.0f }
            },
            {
              .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
              .queueFamilyIndex = gulkan_queue_get_family_index (self->transfer_queue),
              .queueCount = 1,
              .pQueuePriorities = (const float[]) { 0.8f }
            },
          }
        :
          (VkDeviceQueueCreateInfo[]) {
            {
              .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
              .queueFamilyIndex = gulkan_queue_get_family_index(self->graphics_queue),
              .queueCount = 1,
              .pQueuePriorities = (const float[]) { 1.0f }
            },
          },
      .enabledExtensionCount = num_enabled,
      .ppEnabledExtensionNames = (const char* const*) extension_names,
      .pEnabledFeatures = &physical_device_features
    };

  VkResult res = vkCreateDevice (self->physical_device,
                                &device_info, NULL, &self->device);
  vk_check_error ("vkCreateDevice", res, FALSE);

  if (!gulkan_queue_initialize (self->graphics_queue))
      return FALSE;

  if (self->has_transfer_queue)
    {
      if (!gulkan_queue_initialize (self->transfer_queue))
        return FALSE;
    }

  if (num_enabled > 0)
    {
      for (uint32_t i = 0; i < num_enabled; i++)
        g_free (extension_names[i]);
    }
  g_free (extension_names);

  return TRUE;
}

gboolean
gulkan_device_memory_type_from_properties (
  GulkanDevice         *self,
  uint32_t              memory_type_bits,
  VkMemoryPropertyFlags memory_property_flags,
  uint32_t             *type_index_out)
{
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
  {
    if ((memory_type_bits & 1) == 1)
    {
      if ((self->memory_properties.memoryTypes[i].propertyFlags
           & memory_property_flags) == memory_property_flags)
      {
        *type_index_out = i;
        return TRUE;
      }
    }
    memory_type_bits >>= 1;
  }

  /* Could not find matching memory type.*/
  return FALSE;
}

VkDevice
gulkan_device_get_handle (GulkanDevice *self)
{
  return self->device;
}

VkPhysicalDevice
gulkan_device_get_physical_handle (GulkanDevice *self)
{
  return self->physical_device;
}

GulkanQueue*
gulkan_device_get_graphics_queue (GulkanDevice *self)
{
  return self->graphics_queue;
}

GulkanQueue*
gulkan_device_get_transfer_queue (GulkanDevice *self)
{
  if (self->has_transfer_queue)
    return self->transfer_queue;
  else
    return self->graphics_queue;
}

gboolean
gulkan_device_get_memory_fd (GulkanDevice  *self,
                             VkDeviceMemory image_memory,
                             int           *fd)
{
  if (!self->extVkGetMemoryFdKHR)
    self->extVkGetMemoryFdKHR =
      (PFN_vkGetMemoryFdKHR)
        vkGetDeviceProcAddr (self->device, "vkGetMemoryFdKHR");

  if (!self->extVkGetMemoryFdKHR)
    {
      g_printerr ("Gulkan Device: Could not load vkGetMemoryFdKHR\n");
      return FALSE;
    }

  VkMemoryGetFdInfoKHR vkFDInfo =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
    .memory = image_memory,
    .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
  };

  if (self->extVkGetMemoryFdKHR (self->device, &vkFDInfo, fd) != VK_SUCCESS)
    {
      g_printerr ("Gulkan Device: Could not get file descriptor for memory!\n");
      g_object_unref (self);
      return FALSE;
    }
  return TRUE;
}

void
gulkan_device_wait_idle (GulkanDevice *self)
{
  if (self->device != VK_NULL_HANDLE)
    vkDeviceWaitIdle (self->device);
}

void
gulkan_device_print_memory_properties (GulkanDevice *self)
{
  VkPhysicalDeviceMemoryProperties *props = &self->memory_properties;

  g_print ("\n= VkPhysicalDeviceMemoryProperties =\n");
  for (uint32_t i = 0; i < props->memoryTypeCount; i++)
    {
      VkMemoryType *type = &props->memoryTypes[i];
      g_print ("\nVkMemoryType %d: heapIndex %d\n", i, type->heapIndex);

      if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        g_print ("+ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");
      if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        g_print ("+ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n");
      if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        g_print ("+ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");
      if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        g_print ("+ VK_MEMORY_PROPERTY_HOST_CACHED_BIT\n");
      if (type->propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        g_print ("+ VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n");
      if (type->propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        g_print ("+ VK_MEMORY_PROPERTY_PROTECTED_BIT\n");
#ifdef VK_AMD_device_coherent_memory
      if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
        g_print ("+ VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD\n");
      if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
        g_print ("+ VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD\n");
#endif
    }

  for (uint32_t i = 0; i < props->memoryHeapCount; i++)
    {
      VkMemoryHeap *heap = &props->memoryHeaps[i];
      g_print ("\nVkMemoryHeap %d: size %ld MB\n", i, heap->size / 1024 / 1024);
      if (heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        g_print ("+ VK_MEMORY_HEAP_DEVICE_LOCAL_BIT\n");
      if (heap->flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
        g_print ("+ VK_MEMORY_HEAP_MULTI_INSTANCE_BIT\n");
    }

  g_print ("\n====================================\n");
}

void
gulkan_device_print_memory_budget (GulkanDevice *self)
{
#ifdef VK_EXT_memory_budget
  VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
  };

  VkPhysicalDeviceMemoryProperties2 props = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
    .pNext = &budget,
    .memoryProperties = self->memory_properties,
  };

  vkGetPhysicalDeviceMemoryProperties2 (self->physical_device, &props);

  for (uint32_t i = 0; i < self->memory_properties.memoryHeapCount; i++)
    g_print ("Heap %d: usage %.2f budget %.2f MB\n",
             i,
             budget.heapUsage[i] / 1024.0 / 1024.0,
             budget.heapBudget[i] / 1024.0 / 1024.0);
#else
  g_print ("VK_EXT_memory_budget not supported in the vulkan SDK gulkan was compiled with!\n");
#endif
}

VkDeviceSize
gulkan_device_get_heap_budget (GulkanDevice *self, uint32_t i)
{
#ifdef VK_EXT_memory_budget
  VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
  };

  VkPhysicalDeviceMemoryProperties2 props = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
    .pNext = &budget,
    .memoryProperties = self->memory_properties,
  };

  vkGetPhysicalDeviceMemoryProperties2 (self->physical_device, &props);

  return budget.heapBudget[i];
#else
  g_print ("VK_EXT_memory_budget not supported in the vulkan SDK gulkan was compiled with!\n");
  return 0;
#endif
}

VkPhysicalDeviceProperties *
gulkan_device_get_physical_device_properties (GulkanDevice *self)
{
  return &self->physical_props;
}
