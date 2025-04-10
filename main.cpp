#include <stdio.h>

#include <veronica/typedef.h>
#include "utils/enumutils.h"

#ifndef VOLK_LOADER
#define VOLK_LOADER
#endif

#ifdef VOLK_LOADER
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#else
#include <vulkan/vulkan.h>
#endif

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

#include "utils/ioutils.h"

// std
#include <vector>
#include <algorithm>

void error_fatal(const char *msg, VkResult err)
{
        fprintf(stderr, "Error: %s (Result=%s)\n", msg, MAGIC_ENUM_NAME(err));
        exit(EXIT_FAILURE);
}

struct VrakDriver {
        uint32_t version = 0;
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        uint32_t queue_index = 0;
        VkQueue queue = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        VmaAllocator allocator = VK_NULL_HANDLE;
};

struct VrakSwapchainResourceEXT {
        VkImage image = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
};

typedef struct VrakSwapchainEXT_T {
        VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
        VkSurfaceCapabilitiesKHR capabilities = {};
        uint32_t min_image_count = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        std::vector<VrakSwapchainResourceEXT> resources;
} *VrakSwapchainEXT;

typedef struct VrakBuffer_T {
        VkBuffer vk_buffer = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocation_info = {};
} *VrakBuffer;

typedef struct VrakPipeline_T {
        VkPipeline vk_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;
} *VrakPipeline;

struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
};

static Vertex vertices[] = {
    { {  0.0f,  0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { -0.5f, -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
    { {  0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
};

void swapchain_destroy(const VrakDriver *driver, VrakSwapchainEXT swapchain);

VkPhysicalDevice pick_discrete_device(const std::vector<VkPhysicalDevice>& devices)
{
        for (const auto &device : devices) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                        return device;
        }
        
        assert(std::size(devices) > 0);
        return devices[0];
}

VkResult pick_suitable_surface_format(const VrakDriver *driver, VkSurfaceFormatKHR *p_format)
{
        VkResult err;
        
        uint32_t count;
        err = vkGetPhysicalDeviceSurfaceFormatsKHR(driver->gpu, driver->surface, &count, VK_NULL_HANDLE);
        if (err)
                return err;

        std::vector<VkSurfaceFormatKHR> formats(count);
        err = vkGetPhysicalDeviceSurfaceFormatsKHR(driver->gpu, driver->surface, &count, std::data(formats));
        if (err)
                return err;

        for (const auto &item: formats) {
                switch(item.format) {
                        case VK_FORMAT_R8G8B8A8_SRGB:
                        case VK_FORMAT_B8G8R8A8_SRGB:
                        case VK_FORMAT_R8G8B8A8_UNORM:
                        case VK_FORMAT_B8G8R8A8_UNORM: {
                                *p_format = item;
                                goto TAG_PICK_SUITABLE_SURFACE_FORMAT_END;
                        }
                }
        }

        assert(count >= 1);
        *p_format = formats[0];

        printf("Can't not found suitable surface format, default use first\n");

TAG_PICK_SUITABLE_SURFACE_FORMAT_END:
        return VK_SUCCESS;
}

void find_queue_index(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *p_index)
{
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, VK_NULL_HANDLE);
        
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, std::data(properties));
        
        for (uint32_t i = 0; i < count; i++) {
                VkQueueFamilyProperties property = properties[i];
                VkBool32 supported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
                if ((property.queueFlags & VK_QUEUE_GRAPHICS_BIT) && supported) {
                        *p_index = i;
                        return;
                }
        }

        error_fatal("Can't not found queue to support present", VK_ERROR_INITIALIZATION_FAILED);
}

VkResult load_shader_module(VkDevice device, const char *path, VkShaderModule *p_shader_module)
{
        char *buf;
        size_t size;
        VkResult err;

        buf = io_bytebuf_read(path, &size);

        VkShaderModuleCreateInfo shader_module_create_info = {
                /* sType */ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                /* pNext */ VK_NULL_HANDLE,
                /* flags */ 0,
                /* codeSize */ size,
                /* pCode */ reinterpret_cast<uint32_t *>(buf),
        };

        err = vkCreateShaderModule(device, &shader_module_create_info, VK_NULL_HANDLE, p_shader_module);

        io_bytebuf_free(buf);

        return err;
}

VkResult command_buffer_alloc(const VrakDriver *driver, VkCommandBuffer *p_command_buffer)
{
        VkCommandBufferAllocateInfo allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = driver->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        return vkAllocateCommandBuffers(driver->device, &allocate_info, p_command_buffer);
}

void command_buffer_free(const VrakDriver* driver, VkCommandBuffer command_buffer)
{
        vkFreeCommandBuffers(driver->device, driver->command_pool, 1, &command_buffer);
}

void cmd_begin(VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        };

        vkBeginCommandBuffer(command_buffer, &begin_info);
}

void cmd_end(VkCommandBuffer command_buffer)
{
        vkEndCommandBuffer(command_buffer);
}

VkResult buffer_create(const VrakDriver* driver, VkDeviceSize size, VrakBuffer *p_buffer)
{
        VkBufferCreateInfo buffer_ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VmaAllocationCreateInfo allocation_info = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        VrakBuffer tmp = memnew<VrakBuffer_T>();
        tmp->size = size;
        *p_buffer = tmp;

        return vmaCreateBuffer(driver->allocator, &buffer_ci, &allocation_info, &tmp->vk_buffer, &tmp->allocation, &tmp->allocation_info);
}

void buffer_destroy(VrakDriver* driver, VrakBuffer buffer)
{
        vmaDestroyBuffer(driver->allocator, buffer->vk_buffer, buffer->allocation);
        memdel(buffer);
}

void memory_read(const VrakDriver* driver, VrakBuffer buffer, size_t size, void* dst)
{
        void* src;
        vmaMapMemory(driver->allocator, buffer->allocation, &src);
        memcpy(dst, src, size);
        vmaUnmapMemory(driver->allocator, buffer->allocation);
}

void memory_write(const VrakDriver *driver, VrakBuffer buffer, size_t size, void *src)
{
        void* dst;
        vmaMapMemory(driver->allocator, buffer->allocation, &dst);
        memcpy(src, dst, size);
        vmaUnmapMemory(driver->allocator, buffer->allocation);
}

void memory_image_barrier(VkCommandBuffer command_buffer,
                          VkImage image,
                          VkAccessFlags src,
                          VkAccessFlags dst,
                          VkImageLayout old_layout,
                          VkImageLayout new_layout)
{
        VkImageSubresourceRange subresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = src,
            .dstAccessMask = dst,
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = subresource,
        };

        vkCmdPipelineBarrier(
          command_buffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          0,
          0,
          VK_NULL_HANDLE,
          0,
          VK_NULL_HANDLE,
          1,
          &barrier
        );
}

VkResult pipeline_create(const VrakDriver *driver, VrakPipeline *p_pipeline)
{
        VkResult err;
        VrakPipeline tmp = VK_NULL_HANDLE;
        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        VkShaderModule fragment_shader_module = VK_NULL_HANDLE;
        VkPipeline pipeline;
        VkGraphicsPipelineCreateInfo pipeline_ci = {};

        tmp = memnew<VrakPipeline_T>();

        if ((err = load_shader_module(driver->device, "shaders/spir-v/simple_vertex.spv", &vertex_shader_module)))
                goto TAG_PIPELINE_CREATE_OUT;

        if ((err = load_shader_module(driver->device, "shaders/spir-v/simple_fragment.spv", &fragment_shader_module)))
                goto TAG_PIPELINE_CREATE_OUT;

        pipeline_ci = {
             .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR,
        };

        if ((err = vkCreateGraphicsPipelines(driver->device, VK_NULL_HANDLE, 1, &pipeline_ci, VK_NULL_HANDLE, &pipeline)))
                goto TAG_PIPELINE_CREATE_OUT;

        tmp->vk_pipeline = pipeline;
        *p_pipeline = tmp;

TAG_PIPELINE_CREATE_OUT:
        if (err) {
                memdel(tmp);
        }

        if (vertex_shader_module != VK_NULL_HANDLE)
                vkDestroyShaderModule(driver->device, vertex_shader_module, VK_NULL_HANDLE);

        if (fragment_shader_module != VK_NULL_HANDLE)
                vkDestroyShaderModule(driver->device, fragment_shader_module, VK_NULL_HANDLE);

        return err;
}

void pipeline_destroy(const VrakDriver *driver, VrakPipeline pipeline)
{
        if (pipeline == VK_NULL_HANDLE || pipeline->vk_pipeline == VK_NULL_HANDLE)
                return;

        if (pipeline->vk_pipeline_layout != VK_NULL_HANDLE)
                vkDestroyPipelineLayout(driver->device, pipeline->vk_pipeline_layout, VK_NULL_HANDLE);

        vkDestroyPipeline(driver->device, pipeline->vk_pipeline, VK_NULL_HANDLE);
        memdel(pipeline);
}

VrakDriver *vrak_driver_initialize(GLFWwindow *window)
{
        VkResult err;

        VrakDriver *driver = memnew<VrakDriver>();
        
#ifdef VOLK_LOADER
        /*
         * initialize volk loader to dynamic load about instance function
         * api pointer.
         */
        if ((err = volkInitialize()))
                error_fatal("Failed to initialize volk loader", err);
#endif
        
        if ((err = vkEnumerateInstanceVersion(&driver->version)))
                error_fatal("Can't not get instance version", err);
        
        printf("Vulkan %u.%u.%u\n",
               VK_VERSION_MAJOR(driver->version),
               VK_VERSION_MINOR(driver->version),
               VK_VERSION_PATCH(driver->version));
        
        VkApplicationInfo info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Veronak Engine",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Veronak Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = driver->version
        };

        uint32_t count;
        const char **required = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char *> extensions;

        for (int i = 0; i < count; ++i)
                extensions.push_back(required[i]);

        std::vector<const char *> layers = {
                "VK_LAYER_KHRONOS_validation"
        };

        VkInstanceCreateInfo instance_ci = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &info,
            .enabledLayerCount = (uint32_t) std::size(layers),
            .ppEnabledLayerNames = std::data(layers),
            .enabledExtensionCount = (uint32_t) std::size(extensions),
            .ppEnabledExtensionNames = std::data(extensions),
        };

        if ((err = vkCreateInstance(&instance_ci, VK_NULL_HANDLE, &driver->instance)))
                error_fatal("Failed to create instance", err);

#ifdef VOLK_LOADER
        volkLoadInstance(driver->instance);
#endif

        if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, VK_NULL_HANDLE)))
                error_fatal("Failed to enumerate physical device list count", err);
        
        printf("Enumerate vulkan physical device count: %u\n", count);
        
        std::vector<VkPhysicalDevice> devices(count);
        if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, std::data(devices))))
                error_fatal("Failed to enumerate physical device list data", err);
        
        driver->gpu = pick_discrete_device(devices);

        if ((err = glfwCreateWindowSurface(driver->instance, window, VK_NULL_HANDLE, &driver->surface)))
                error_fatal("Failed to create surface", err);

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(driver->gpu, &properties);
        printf("Use GPU %s\n", properties.deviceName);
        
        float priorities = 1.0f;
        
        find_queue_index(driver->gpu, driver->surface, &driver->queue_index);
        
        VkDeviceQueueCreateInfo queue_ci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = driver->queue_index,
            .queueCount = 1,
            .pQueuePriorities = &priorities,
        };

        std::vector<const char *> device_extensions = {
                "VK_KHR_swapchain"
        };

        VkDeviceCreateInfo device_ci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_ci,
            .enabledExtensionCount = (uint32_t) std::size(device_extensions),
            .ppEnabledExtensionNames = std::data(device_extensions),
        };

        if ((err = vkCreateDevice(driver->gpu, &device_ci, VK_NULL_HANDLE, &driver->device)))
                error_fatal("Failed to create logic device", err);
        
#ifdef VOLK_LOADER
        volkLoadDevice(driver->device);
#endif
        
        vkGetDeviceQueue(driver->device, driver->queue_index, 0, &driver->queue);
        
        VkCommandPoolCreateInfo command_pool_ci = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = driver->queue_index,
        };
        
        if ((err = vkCreateCommandPool(driver->device, &command_pool_ci, VK_NULL_HANDLE, &driver->command_pool)))
                error_fatal("Failed to create command pool", err);
        
        VmaVulkanFunctions functions = {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        };
        
        VmaAllocatorCreateInfo allocator_ci = {
            .physicalDevice = driver->gpu,
            .device = driver->device,
            .pVulkanFunctions = &functions,
            .instance = driver->instance,
        };
        
        if ((err = vmaCreateAllocator(&allocator_ci, &driver->allocator)))
                error_fatal("Failed to create VMA allocator", err);

        return driver;
}

void vrak_driver_destroy(VrakDriver* driver)
{
        vmaDestroyAllocator(driver->allocator);
        vkDestroyCommandPool(driver->device, driver->command_pool, VK_NULL_HANDLE);
        vkDestroyDevice(driver->device, VK_NULL_HANDLE);
        vkDestroySurfaceKHR(driver->instance, driver->surface, VK_NULL_HANDLE);
        vkDestroyInstance(driver->instance, VK_NULL_HANDLE);
        memdel(driver);
}

VkResult image_view2d_create(const VrakDriver *driver, VkImage image, VkFormat format, VkImageView *p_view2d)
{
        VkImageViewCreateInfo image_view2d_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        return vkCreateImageView(driver->device, &image_view2d_ci, VK_NULL_HANDLE, p_view2d);
}

void image_view2d_destroy(const VrakDriver *driver, VkImageView view)
{
        vkDestroyImageView(driver->device, view, VK_NULL_HANDLE);
}

VkResult swapchain_create(VrakDriver *driver, VrakSwapchainEXT *p_swapchain)
{
        VkResult err;
        VrakSwapchainEXT tmp = VK_NULL_HANDLE;
        uint32_t min = 0;
        uint32_t max = 0;
        VkSurfaceFormatKHR surface_format = {};
        VkSwapchainCreateInfoKHR swapchain_ci = {};
        uint32_t count = 0;
        std::vector<VkImage> images;

        tmp = memnew<VrakSwapchainEXT_T>();

        if ((err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(driver->gpu, driver->surface, &tmp->capabilities)))
                goto TAG_SWAPCHAIN_CREATE_OUT;

        min = tmp->capabilities.minImageCount;
        max = tmp->capabilities.maxImageCount;
        tmp->min_image_count = std::clamp(min + 1, min, max);

        *p_swapchain = tmp;

        if ((err = pick_suitable_surface_format(driver, &surface_format)))
                goto TAG_SWAPCHAIN_CREATE_OUT;

        tmp->format = surface_format.format;
        tmp->color_space = surface_format.colorSpace;

        swapchain_ci = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = driver->surface,
            .minImageCount = tmp->min_image_count,
            .imageFormat = tmp->format,
            .imageColorSpace = tmp->color_space,
            .imageExtent = tmp->capabilities.currentExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = tmp->capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };

        if ((err = vkCreateSwapchainKHR(driver->device, &swapchain_ci, VK_NULL_HANDLE, &tmp->vk_swapchain)))
                goto TAG_SWAPCHAIN_CREATE_OUT;

        if ((err = vkGetSwapchainImagesKHR(driver->device, tmp->vk_swapchain, &count, VK_NULL_HANDLE)))
                goto TAG_SWAPCHAIN_CREATE_OUT;

        images.resize(count);
        if ((err = vkGetSwapchainImagesKHR(driver->device, tmp->vk_swapchain, &count, std::data(images))))
                goto TAG_SWAPCHAIN_CREATE_OUT;

        tmp->resources.resize(tmp->min_image_count);
        for (int i = 0; i < tmp->min_image_count; ++i) {
                tmp->resources[i].image = images[i];
                if ((err = image_view2d_create(driver, tmp->resources[i].image, tmp->format, &(tmp->resources[i].image_view))))
                        goto TAG_SWAPCHAIN_CREATE_OUT;
        }

TAG_SWAPCHAIN_CREATE_OUT:
        if (err) {
                swapchain_destroy(driver, tmp);
        }

        return err;
}

void swapchain_destroy(const VrakDriver *driver, VrakSwapchainEXT swapchain)
{
        if (swapchain == VK_NULL_HANDLE || swapchain->vk_swapchain == VK_NULL_HANDLE)
                return;

        for (int i = 0; i < swapchain->min_image_count; ++i) {
                if (swapchain->resources[i].image_view != VK_NULL_HANDLE)
                        image_view2d_destroy(driver, swapchain->resources[i].image_view);
        }

        vkDestroySwapchainKHR(driver->device, swapchain->vk_swapchain, VK_NULL_HANDLE);
        memdel(swapchain);
}

int main()
{
        VkResult err;

        /*
         * close stdout and stderr write to buf, let direct
         * output.
         */
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWwindow* window = VK_NULL_HANDLE;
        VrakDriver* driver = VK_NULL_HANDLE;
        VrakSwapchainEXT swapchain = VK_NULL_HANDLE;
        VrakBuffer vertex_buffer = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VrakPipeline pipeline = VK_NULL_HANDLE;

        window = glfwCreateWindow(800, 600, "Veronak Cube", nullptr, nullptr);
        driver = vrak_driver_initialize(window);
        swapchain_create(driver, &swapchain);

        // create vertex buffer
        err = buffer_create(driver, sizeof(vertices), &vertex_buffer);
        if (err)
                error_fatal("Failed to create vertex buffer", err);

        memory_write(driver, vertex_buffer, sizeof(vertices), (void*) vertices);

        command_buffer_alloc(driver, &command_buffer);
        pipeline_create(driver, &pipeline);

        while (!glfwWindowShouldClose(window)) {
                cmd_begin(command_buffer);
                cmd_end(command_buffer);
                glfwPollEvents();
        }

        pipeline_destroy(driver, pipeline);
        command_buffer_free(driver, command_buffer);
        buffer_destroy(driver, vertex_buffer);
        swapchain_destroy(driver, swapchain);
        vrak_driver_destroy(driver);
        glfwDestroyWindow(window);

        glfwTerminate();
        
        return 0;
}
