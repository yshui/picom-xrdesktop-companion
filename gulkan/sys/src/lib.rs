// Generated by gir (https://github.com/gtk-rs/gir @ 2358cc24efd2)
// from ../../gir-files (@ eab91ba8f88b)
// from ../../xrd-gir-files (@ 3896faadf111)
// DO NOT EDIT

#![allow(non_camel_case_types, non_upper_case_globals, non_snake_case)]
#![allow(
    clippy::approx_constant,
    clippy::type_complexity,
    clippy::unreadable_literal,
    clippy::upper_case_acronyms
)]
#![cfg_attr(feature = "dox", feature(doc_cfg))]

#[allow(unused_imports)]
use libc::{
    c_char, c_double, c_float, c_int, c_long, c_short, c_uchar, c_uint, c_ulong, c_ushort, c_void,
    intptr_t, size_t, ssize_t, uintptr_t, FILE,
};

#[allow(unused_imports)]
use glib::{gboolean, gconstpointer, gpointer, GType};

// Records
#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanBufferClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanBufferClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanBufferClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanClientClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanClientClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanClientClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanCmdBufferClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanCmdBufferClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanCmdBufferClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanDescriptorPoolClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanDescriptorPoolClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanDescriptorPoolClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanDeviceClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanDeviceClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanDeviceClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanFrameBufferClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanFrameBufferClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanFrameBufferClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanInstanceClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanInstanceClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanInstanceClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanQueueClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanQueueClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanQueueClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanRenderPassClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanRenderPassClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanRenderPassClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanRendererClass {
    pub parent: gobject::GObjectClass,
    pub draw: Option<unsafe extern "C" fn(*mut GulkanRenderer) -> gboolean>,
}

impl ::std::fmt::Debug for GulkanRendererClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanRendererClass @ {self:p}"))
            .field("parent", &self.parent)
            .field("draw", &self.draw)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanSwapchainClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanSwapchainClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanSwapchainClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanSwapchainRendererClass {
    pub parent: GulkanRendererClass,
    pub init_draw_cmd:
        Option<unsafe extern "C" fn(*mut GulkanSwapchainRenderer, vulkan::VkCommandBuffer)>,
    pub init_pipeline:
        Option<unsafe extern "C" fn(*mut GulkanSwapchainRenderer, gconstpointer) -> gboolean>,
}

impl ::std::fmt::Debug for GulkanSwapchainRendererClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanSwapchainRendererClass @ {self:p}"))
            .field("parent", &self.parent)
            .field("init_draw_cmd", &self.init_draw_cmd)
            .field("init_pipeline", &self.init_pipeline)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanTextureClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanTextureClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanTextureClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanUniformBufferClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanUniformBufferClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanUniformBufferClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanVertexBufferClass {
    pub parent_class: gobject::GObjectClass,
}

impl ::std::fmt::Debug for GulkanVertexBufferClass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanVertexBufferClass @ {self:p}"))
            .field("parent_class", &self.parent_class)
            .finish()
    }
}

// Classes
#[repr(C)]
pub struct GulkanBuffer {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanBuffer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanBuffer @ {self:p}")).finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanClient {
    pub parent_instance: gobject::GObject,
}

impl ::std::fmt::Debug for GulkanClient {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanClient @ {self:p}"))
            .field("parent_instance", &self.parent_instance)
            .finish()
    }
}

#[repr(C)]
pub struct GulkanCmdBuffer {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanCmdBuffer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanCmdBuffer @ {self:p}"))
            .finish()
    }
}

#[repr(C)]
pub struct GulkanDescriptorPool {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanDescriptorPool {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanDescriptorPool @ {self:p}"))
            .finish()
    }
}

#[repr(C)]
pub struct GulkanDevice {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanDevice {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanDevice @ {self:p}")).finish()
    }
}

#[repr(C)]
pub struct GulkanFrameBuffer {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanFrameBuffer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanFrameBuffer @ {self:p}"))
            .finish()
    }
}

#[repr(C)]
pub struct GulkanInstance {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanInstance {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanInstance @ {self:p}"))
            .finish()
    }
}

#[repr(C)]
pub struct GulkanQueue {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanQueue {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanQueue @ {self:p}")).finish()
    }
}

#[repr(C)]
pub struct GulkanRenderPass {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanRenderPass {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanRenderPass @ {self:p}"))
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanRenderer {
    pub parent_instance: gobject::GObject,
}

impl ::std::fmt::Debug for GulkanRenderer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanRenderer @ {self:p}"))
            .field("parent_instance", &self.parent_instance)
            .finish()
    }
}

#[repr(C)]
pub struct GulkanSwapchain {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanSwapchain {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanSwapchain @ {self:p}"))
            .finish()
    }
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct GulkanSwapchainRenderer {
    pub parent_instance: GulkanRenderer,
}

impl ::std::fmt::Debug for GulkanSwapchainRenderer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanSwapchainRenderer @ {self:p}"))
            .field("parent_instance", &self.parent_instance)
            .finish()
    }
}

#[repr(C)]
pub struct GulkanTexture {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanTexture {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanTexture @ {self:p}"))
            .finish()
    }
}

#[repr(C)]
pub struct GulkanUniformBuffer {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanUniformBuffer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanUniformBuffer @ {self:p}"))
            .finish()
    }
}

#[repr(C)]
pub struct GulkanVertexBuffer {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl ::std::fmt::Debug for GulkanVertexBuffer {
    fn fmt(&self, f: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        f.debug_struct(&format!("GulkanVertexBuffer @ {self:p}"))
            .finish()
    }
}

#[link(name = "gulkan-0.15", kind = "static", modifiers = "-bundle")]
extern "C" {

    //=========================================================================
    // GulkanBuffer
    //=========================================================================
    pub fn gulkan_buffer_get_type() -> GType;
    pub fn gulkan_buffer_new(
        device: *mut GulkanDevice,
        size: vulkan::VkDeviceSize,
        usage: vulkan::VkBufferUsageFlags,
        properties: vulkan::VkMemoryPropertyFlags,
    ) -> *mut GulkanBuffer;
    pub fn gulkan_buffer_new_from_data(
        device: *mut GulkanDevice,
        data: *mut c_void,
        size: vulkan::VkDeviceSize,
        usage: vulkan::VkBufferUsageFlags,
        properties: vulkan::VkMemoryPropertyFlags,
    ) -> *mut GulkanBuffer;
    pub fn gulkan_buffer_get_handle(self_: *mut GulkanBuffer) -> vulkan::VkBuffer;
    pub fn gulkan_buffer_get_memory_handle(self_: *mut GulkanBuffer) -> vulkan::VkDeviceMemory;
    pub fn gulkan_buffer_map(self_: *mut GulkanBuffer, data: *mut *mut c_void) -> gboolean;
    pub fn gulkan_buffer_unmap(self_: *mut GulkanBuffer);
    pub fn gulkan_buffer_upload(
        self_: *mut GulkanBuffer,
        data: *mut c_void,
        size: vulkan::VkDeviceSize,
    ) -> gboolean;

    //=========================================================================
    // GulkanClient
    //=========================================================================
    pub fn gulkan_client_get_type() -> GType;
    pub fn gulkan_client_new() -> *mut GulkanClient;
    pub fn gulkan_client_new_from_extensions(
        instance_ext_list: *mut glib::GSList,
        device_ext_list: *mut glib::GSList,
    ) -> *mut GulkanClient;
    pub fn gulkan_client_get_external_memory_device_extensions() -> *mut glib::GSList;
    pub fn gulkan_client_get_external_memory_instance_extensions() -> *mut glib::GSList;
    pub fn gulkan_client_get_device(self_: *mut GulkanClient) -> *mut GulkanDevice;
    pub fn gulkan_client_get_device_handle(self_: *mut GulkanClient) -> vulkan::VkDevice;
    pub fn gulkan_client_get_instance(self_: *mut GulkanClient) -> *mut GulkanInstance;
    pub fn gulkan_client_get_instance_handle(self_: *mut GulkanClient) -> vulkan::VkInstance;
    pub fn gulkan_client_get_physical_device_handle(
        self_: *mut GulkanClient,
    ) -> vulkan::VkPhysicalDevice;

    //=========================================================================
    // GulkanCmdBuffer
    //=========================================================================
    pub fn gulkan_cmd_buffer_get_type() -> GType;
    pub fn gulkan_cmd_buffer_begin(self_: *mut GulkanCmdBuffer) -> gboolean;
    pub fn gulkan_cmd_buffer_get_handle(self_: *mut GulkanCmdBuffer) -> vulkan::VkCommandBuffer;

    //=========================================================================
    // GulkanDescriptorPool
    //=========================================================================
    pub fn gulkan_descriptor_pool_get_type() -> GType;
    pub fn gulkan_descriptor_pool_new(
        device: vulkan::VkDevice,
        bindings: *const vulkan::VkDescriptorSetLayoutBinding,
        binding_count: u32,
        pool_sizes: *const vulkan::VkDescriptorPoolSize,
        pool_size_count: u32,
        set_count: u32,
    ) -> *mut GulkanDescriptorPool;
    pub fn gulkan_descriptor_pool_new_from_layout(
        device: vulkan::VkDevice,
        layout: vulkan::VkDescriptorSetLayout,
        pool_sizes: *const vulkan::VkDescriptorPoolSize,
        pool_size_count: u32,
        set_count: u32,
    ) -> *mut GulkanDescriptorPool;
    pub fn gulkan_descriptor_pool_allocate_sets(
        self_: *mut GulkanDescriptorPool,
        count: u32,
        sets: *mut vulkan::VkDescriptorSet,
    ) -> gboolean;
    pub fn gulkan_descriptor_pool_get_pipeline_layout(
        self_: *mut GulkanDescriptorPool,
    ) -> vulkan::VkPipelineLayout;

    //=========================================================================
    // GulkanDevice
    //=========================================================================
    pub fn gulkan_device_get_type() -> GType;
    pub fn gulkan_device_new() -> *mut GulkanDevice;
    pub fn gulkan_device_create(
        self_: *mut GulkanDevice,
        instance: *mut GulkanInstance,
        device: vulkan::VkPhysicalDevice,
        extensions: *mut glib::GSList,
    ) -> gboolean;
    pub fn gulkan_device_get_graphics_queue(self_: *mut GulkanDevice) -> *mut GulkanQueue;
    pub fn gulkan_device_get_handle(self_: *mut GulkanDevice) -> vulkan::VkDevice;
    pub fn gulkan_device_get_heap_budget(self_: *mut GulkanDevice, i: u32) -> vulkan::VkDeviceSize;
    pub fn gulkan_device_get_memory_fd(
        self_: *mut GulkanDevice,
        image_memory: vulkan::VkDeviceMemory,
        fd: *mut c_int,
    ) -> gboolean;
    pub fn gulkan_device_get_physical_device_properties(
        self_: *mut GulkanDevice,
    ) -> *mut vulkan::VkPhysicalDeviceProperties;
    pub fn gulkan_device_get_physical_handle(self_: *mut GulkanDevice) -> vulkan::VkPhysicalDevice;
    pub fn gulkan_device_get_transfer_queue(self_: *mut GulkanDevice) -> *mut GulkanQueue;
    pub fn gulkan_device_memory_type_from_properties(
        self_: *mut GulkanDevice,
        memory_type_bits: u32,
        memory_property_flags: vulkan::VkMemoryPropertyFlags,
        type_index_out: *mut u32,
    ) -> gboolean;
    pub fn gulkan_device_print_memory_budget(self_: *mut GulkanDevice);
    pub fn gulkan_device_print_memory_properties(self_: *mut GulkanDevice);
    pub fn gulkan_device_wait_idle(self_: *mut GulkanDevice);

    //=========================================================================
    // GulkanFrameBuffer
    //=========================================================================
    pub fn gulkan_frame_buffer_get_type() -> GType;
    pub fn gulkan_frame_buffer_new(
        device: *mut GulkanDevice,
        render_pass: *mut GulkanRenderPass,
        extent: vulkan::VkExtent2D,
        sample_count: vulkan::VkSampleCountFlagBits,
        color_format: vulkan::VkFormat,
        use_depth: gboolean,
    ) -> *mut GulkanFrameBuffer;
    pub fn gulkan_frame_buffer_new_from_image(
        device: *mut GulkanDevice,
        render_pass: *mut GulkanRenderPass,
        color_image: vulkan::VkImage,
        extent: vulkan::VkExtent2D,
        color_format: vulkan::VkFormat,
    ) -> *mut GulkanFrameBuffer;
    pub fn gulkan_frame_buffer_new_from_image_with_depth(
        device: *mut GulkanDevice,
        render_pass: *mut GulkanRenderPass,
        color_image: vulkan::VkImage,
        extent: vulkan::VkExtent2D,
        sample_count: vulkan::VkSampleCountFlagBits,
        color_format: vulkan::VkFormat,
    ) -> *mut GulkanFrameBuffer;
    pub fn gulkan_frame_buffer_get_color_image(self_: *mut GulkanFrameBuffer) -> vulkan::VkImage;
    pub fn gulkan_frame_buffer_get_handle(self_: *mut GulkanFrameBuffer) -> vulkan::VkFramebuffer;

    //=========================================================================
    // GulkanInstance
    //=========================================================================
    pub fn gulkan_instance_get_type() -> GType;
    pub fn gulkan_instance_new() -> *mut GulkanInstance;
    pub fn gulkan_instance_create(
        self_: *mut GulkanInstance,
        required_extensions: *mut glib::GSList,
    ) -> gboolean;
    pub fn gulkan_instance_get_handle(self_: *mut GulkanInstance) -> vulkan::VkInstance;

    //=========================================================================
    // GulkanQueue
    //=========================================================================
    pub fn gulkan_queue_get_type() -> GType;
    pub fn gulkan_queue_new(device: *mut GulkanDevice, family_index: u32) -> *mut GulkanQueue;
    pub fn gulkan_queue_free_cmd_buffer(self_: *mut GulkanQueue, cmd_buffer: *mut GulkanCmdBuffer);
    pub fn gulkan_queue_get_command_pool(self_: *mut GulkanQueue) -> vulkan::VkCommandPool;
    pub fn gulkan_queue_get_family_index(self_: *mut GulkanQueue) -> u32;
    pub fn gulkan_queue_get_handle(self_: *mut GulkanQueue) -> vulkan::VkQueue;
    pub fn gulkan_queue_get_pool_mutex(self_: *mut GulkanQueue) -> *mut glib::GMutex;
    pub fn gulkan_queue_initialize(self_: *mut GulkanQueue) -> gboolean;
    pub fn gulkan_queue_request_cmd_buffer(self_: *mut GulkanQueue) -> *mut GulkanCmdBuffer;
    pub fn gulkan_queue_submit(
        self_: *mut GulkanQueue,
        cmd_buffer: *mut GulkanCmdBuffer,
    ) -> gboolean;
    pub fn gulkan_queue_supports_surface(
        self_: *mut GulkanQueue,
        surface: vulkan::VkSurfaceKHR,
    ) -> gboolean;

    //=========================================================================
    // GulkanRenderPass
    //=========================================================================
    pub fn gulkan_render_pass_get_type() -> GType;
    pub fn gulkan_render_pass_new(
        device: *mut GulkanDevice,
        samples: vulkan::VkSampleCountFlagBits,
        color_format: vulkan::VkFormat,
        final_color_layout: vulkan::VkImageLayout,
        use_depth: gboolean,
    ) -> *mut GulkanRenderPass;
    pub fn gulkan_render_pass_begin(
        self_: *mut GulkanRenderPass,
        extent: vulkan::VkExtent2D,
        clear_color: vulkan::VkClearColorValue,
        frame_buffer: *mut GulkanFrameBuffer,
        cmd_buffer: vulkan::VkCommandBuffer,
    );
    pub fn gulkan_render_pass_get_handle(self_: *mut GulkanRenderPass) -> vulkan::VkRenderPass;

    //=========================================================================
    // GulkanRenderer
    //=========================================================================
    pub fn gulkan_renderer_get_type() -> GType;
    pub fn gulkan_renderer_create_shader_module(
        self_: *mut GulkanRenderer,
        resource_name: *const c_char,
        module: *mut vulkan::VkShaderModule,
    ) -> gboolean;
    pub fn gulkan_renderer_draw(self_: *mut GulkanRenderer) -> gboolean;
    pub fn gulkan_renderer_get_aspect(self_: *mut GulkanRenderer) -> c_float;
    pub fn gulkan_renderer_get_client(self_: *mut GulkanRenderer) -> *mut GulkanClient;
    pub fn gulkan_renderer_get_extent(self_: *mut GulkanRenderer) -> vulkan::VkExtent2D;
    pub fn gulkan_renderer_get_msec_since_start(self_: *mut GulkanRenderer) -> i64;
    pub fn gulkan_renderer_set_client(self_: *mut GulkanRenderer, client: *mut GulkanClient);
    pub fn gulkan_renderer_set_extent(self_: *mut GulkanRenderer, extent: vulkan::VkExtent2D);

    //=========================================================================
    // GulkanSwapchain
    //=========================================================================
    pub fn gulkan_swapchain_get_type() -> GType;
    pub fn gulkan_swapchain_new(
        client: *mut GulkanClient,
        surface: vulkan::VkSurfaceKHR,
        present_mode: vulkan::VkPresentModeKHR,
        format: vulkan::VkFormat,
        colorspace: vulkan::VkColorSpaceKHR,
    ) -> *mut GulkanSwapchain;
    pub fn gulkan_swapchain_acquire(
        self_: *mut GulkanSwapchain,
        signal_semaphore: vulkan::VkSemaphore,
        index: *mut u32,
    ) -> gboolean;
    pub fn gulkan_swapchain_get_extent(self_: *mut GulkanSwapchain) -> vulkan::VkExtent2D;
    pub fn gulkan_swapchain_get_format(self_: *mut GulkanSwapchain) -> vulkan::VkFormat;
    pub fn gulkan_swapchain_get_images(
        self_: *mut GulkanSwapchain,
        swap_chain_images: *mut vulkan::VkImage,
    );
    pub fn gulkan_swapchain_get_size(self_: *mut GulkanSwapchain) -> u32;
    pub fn gulkan_swapchain_present(
        self_: *mut GulkanSwapchain,
        wait_semaphore: *mut vulkan::VkSemaphore,
        index: u32,
    ) -> gboolean;
    pub fn gulkan_swapchain_reset_surface(
        self_: *mut GulkanSwapchain,
        surface: vulkan::VkSurfaceKHR,
    ) -> gboolean;

    //=========================================================================
    // GulkanSwapchainRenderer
    //=========================================================================
    pub fn gulkan_swapchain_renderer_get_type() -> GType;
    pub fn gulkan_swapchain_renderer_begin_render_pass(
        self_: *mut GulkanSwapchainRenderer,
        clear_color: vulkan::VkClearColorValue,
        index: u32,
    );
    pub fn gulkan_swapchain_renderer_get_cmd_buffer(
        self_: *mut GulkanSwapchainRenderer,
        index: u32,
    ) -> vulkan::VkCommandBuffer;
    pub fn gulkan_swapchain_renderer_get_frame_buffer(
        self_: *mut GulkanSwapchainRenderer,
        index: u32,
    ) -> *mut GulkanFrameBuffer;
    pub fn gulkan_swapchain_renderer_get_render_pass(
        self_: *mut GulkanSwapchainRenderer,
    ) -> *mut GulkanRenderPass;
    pub fn gulkan_swapchain_renderer_get_render_pass_handle(
        self_: *mut GulkanSwapchainRenderer,
    ) -> vulkan::VkRenderPass;
    pub fn gulkan_swapchain_renderer_get_swapchain_size(self_: *mut GulkanSwapchainRenderer)
        -> u32;
    pub fn gulkan_swapchain_renderer_init_draw_cmd_buffers(
        self_: *mut GulkanSwapchainRenderer,
    ) -> gboolean;
    pub fn gulkan_swapchain_renderer_initialize(
        self_: *mut GulkanSwapchainRenderer,
        surface: vulkan::VkSurfaceKHR,
        clear_color: vulkan::VkClearColorValue,
        pipeline_data: gconstpointer,
    ) -> gboolean;
    pub fn gulkan_swapchain_renderer_resize(
        self_: *mut GulkanSwapchainRenderer,
        surface: vulkan::VkSurfaceKHR,
    ) -> gboolean;

    //=========================================================================
    // GulkanTexture
    //=========================================================================
    pub fn gulkan_texture_get_type() -> GType;
    pub fn gulkan_texture_new(
        client: *mut GulkanClient,
        extent: vulkan::VkExtent2D,
        format: vulkan::VkFormat,
    ) -> *mut GulkanTexture;
    pub fn gulkan_texture_new_export_fd(
        client: *mut GulkanClient,
        extent: vulkan::VkExtent2D,
        format: vulkan::VkFormat,
        layout: vulkan::VkImageLayout,
        size: *mut size_t,
        fd: *mut c_int,
    ) -> *mut GulkanTexture;
    pub fn gulkan_texture_new_from_cairo_surface(
        client: *mut GulkanClient,
        surface: *mut cairo::cairo_surface_t,
        format: vulkan::VkFormat,
        layout: vulkan::VkImageLayout,
    ) -> *mut GulkanTexture;
    pub fn gulkan_texture_new_from_dmabuf(
        client: *mut GulkanClient,
        fd: c_int,
        extent: vulkan::VkExtent2D,
        format: vulkan::VkFormat,
    ) -> *mut GulkanTexture;
    pub fn gulkan_texture_new_from_pixbuf(
        client: *mut GulkanClient,
        pixbuf: *mut gdk_pixbuf::GdkPixbuf,
        format: vulkan::VkFormat,
        layout: vulkan::VkImageLayout,
        create_mipmaps: gboolean,
    ) -> *mut GulkanTexture;
    pub fn gulkan_texture_new_mip_levels(
        client: *mut GulkanClient,
        extent: vulkan::VkExtent2D,
        mip_levels: c_uint,
        format: vulkan::VkFormat,
    ) -> *mut GulkanTexture;
    pub fn gulkan_texture_get_extent(self_: *mut GulkanTexture) -> vulkan::VkExtent2D;
    pub fn gulkan_texture_get_format(self_: *mut GulkanTexture) -> vulkan::VkFormat;
    pub fn gulkan_texture_get_image(self_: *mut GulkanTexture) -> vulkan::VkImage;
    pub fn gulkan_texture_get_image_view(self_: *mut GulkanTexture) -> vulkan::VkImageView;
    pub fn gulkan_texture_get_mip_levels(self_: *mut GulkanTexture) -> c_uint;
    pub fn gulkan_texture_record_transfer(
        self_: *mut GulkanTexture,
        cmd_buffer: vulkan::VkCommandBuffer,
        src_layout: vulkan::VkImageLayout,
        dst_layout: vulkan::VkImageLayout,
    );
    pub fn gulkan_texture_record_transfer_full(
        self_: *mut GulkanTexture,
        cmd_buffer: vulkan::VkCommandBuffer,
        src_access_mask: vulkan::VkAccessFlags,
        dst_access_mask: vulkan::VkAccessFlags,
        src_layout: vulkan::VkImageLayout,
        dst_layout: vulkan::VkImageLayout,
        src_stage_mask: vulkan::VkPipelineStageFlags,
        dst_stage_mask: vulkan::VkPipelineStageFlags,
    );
    pub fn gulkan_texture_transfer_layout(
        self_: *mut GulkanTexture,
        src_layout: vulkan::VkImageLayout,
        dst_layout: vulkan::VkImageLayout,
    ) -> gboolean;
    pub fn gulkan_texture_transfer_layout_full(
        self_: *mut GulkanTexture,
        src_access_mask: vulkan::VkAccessFlags,
        dst_access_mask: vulkan::VkAccessFlags,
        src_layout: vulkan::VkImageLayout,
        dst_layout: vulkan::VkImageLayout,
        src_stage_mask: vulkan::VkPipelineStageFlags,
        dst_stage_mask: vulkan::VkPipelineStageFlags,
    ) -> gboolean;
    pub fn gulkan_texture_upload_cairo_surface(
        self_: *mut GulkanTexture,
        surface: *mut cairo::cairo_surface_t,
        layout: vulkan::VkImageLayout,
    ) -> gboolean;
    pub fn gulkan_texture_upload_pixbuf(
        self_: *mut GulkanTexture,
        pixbuf: *mut gdk_pixbuf::GdkPixbuf,
        layout: vulkan::VkImageLayout,
    ) -> gboolean;
    pub fn gulkan_texture_upload_pixels(
        self_: *mut GulkanTexture,
        pixels: *mut u8,
        size: size_t,
        layout: vulkan::VkImageLayout,
    ) -> gboolean;

    //=========================================================================
    // GulkanUniformBuffer
    //=========================================================================
    pub fn gulkan_uniform_buffer_get_type() -> GType;
    pub fn gulkan_uniform_buffer_new(
        device: *mut GulkanDevice,
        size: vulkan::VkDeviceSize,
    ) -> *mut GulkanUniformBuffer;
    pub fn gulkan_uniform_buffer_get_handle(self_: *mut GulkanUniformBuffer) -> vulkan::VkBuffer;
    pub fn gulkan_uniform_buffer_update(self_: *mut GulkanUniformBuffer, s: *mut gpointer);

    //=========================================================================
    // GulkanVertexBuffer
    //=========================================================================
    pub fn gulkan_vertex_buffer_get_type() -> GType;
    pub fn gulkan_vertex_buffer_new() -> *mut GulkanVertexBuffer;
    pub fn gulkan_vertex_buffer_new_from_attribs(
        device: *mut GulkanDevice,
        positions: *const c_float,
        positions_size: size_t,
        colors: *const c_float,
        colors_size: size_t,
        normals: *const c_float,
        normals_size: size_t,
    ) -> *mut GulkanVertexBuffer;
    pub fn gulkan_vertex_buffer_alloc_array(
        self_: *mut GulkanVertexBuffer,
        device: *mut GulkanDevice,
    ) -> gboolean;
    pub fn gulkan_vertex_buffer_alloc_data(
        self_: *mut GulkanVertexBuffer,
        device: *mut GulkanDevice,
        data: *mut c_void,
        size: vulkan::VkDeviceSize,
    ) -> gboolean;
    pub fn gulkan_vertex_buffer_alloc_empty(
        self_: *mut GulkanVertexBuffer,
        device: *mut GulkanDevice,
        multiplier: u32,
    ) -> gboolean;
    pub fn gulkan_vertex_buffer_alloc_index_data(
        self_: *mut GulkanVertexBuffer,
        device: *mut GulkanDevice,
        data: *mut c_void,
        element_size: vulkan::VkDeviceSize,
        element_count: c_uint,
    ) -> gboolean;
    pub fn gulkan_vertex_buffer_append_position_uv(
        self_: *mut GulkanVertexBuffer,
        vec: *mut graphene::graphene_vec4_t,
        u: c_float,
        v: c_float,
    );
    pub fn gulkan_vertex_buffer_append_with_color(
        self_: *mut GulkanVertexBuffer,
        vec: *mut graphene::graphene_vec4_t,
        color: *mut graphene::graphene_vec3_t,
    );
    pub fn gulkan_vertex_buffer_bind_with_offsets(
        self_: *mut GulkanVertexBuffer,
        cmd_buffer: vulkan::VkCommandBuffer,
    );
    pub fn gulkan_vertex_buffer_draw(
        self_: *mut GulkanVertexBuffer,
        cmd_buffer: vulkan::VkCommandBuffer,
    );
    pub fn gulkan_vertex_buffer_draw_indexed(
        self_: *mut GulkanVertexBuffer,
        cmd_buffer: vulkan::VkCommandBuffer,
    );
    pub fn gulkan_vertex_buffer_is_initialized(self_: *mut GulkanVertexBuffer) -> gboolean;
    pub fn gulkan_vertex_buffer_map_array(self_: *mut GulkanVertexBuffer) -> gboolean;
    pub fn gulkan_vertex_buffer_reset(self_: *mut GulkanVertexBuffer);

    //=========================================================================
    // Other functions
    //=========================================================================
    pub fn gulkan_geometry_append_axes(
        self_: *mut GulkanVertexBuffer,
        center: *mut graphene::graphene_vec4_t,
        length: c_float,
        mat: *mut graphene::graphene_matrix_t,
    );
    pub fn gulkan_geometry_append_plane(
        self_: *mut GulkanVertexBuffer,
        from: *mut graphene::graphene_point_t,
        to: *mut graphene::graphene_point_t,
        mat: *mut graphene::graphene_matrix_t,
    );
    pub fn gulkan_geometry_append_ray(
        self_: *mut GulkanVertexBuffer,
        center: *mut graphene::graphene_vec4_t,
        length: c_float,
        mat: *mut graphene::graphene_matrix_t,
    );
    pub fn gulkan_has_error(
        res: vulkan::VkResult,
        fun: *const c_char,
        file: *const c_char,
        line: c_int,
    ) -> gboolean;

}
