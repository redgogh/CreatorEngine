/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 Red Gogh All rights reserved.                         *|
|*                                                                                  *|
|*    Licensed under the Apache License, Version 2.0 (the "License");               *|
|*    you may not use this file except in compliance with the License.              *|
|*    You may obtain a copy of the License at                                       *|
|*                                                                                  *|
|*        http://www.apache.org/licenses/LICENSE-2.0                                *|
|*                                                                                  *|
|*    Unless required by applicable law or agreed to in writing, software           *|
|*    distributed under the License is distributed on an "AS IS" BASIS,             *|
|*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.      *|
|*    See the License for the specific language governing permissions and           *|
|*    limitations under the License.                                                *|
|*                                                                                  *|
\* -------------------------------------------------------------------------------- */
#include <stdio.h>

#include <vronk/typedef.h>

#define USE_VOLK
#define USE_GLFW

#ifdef USE_VOLK
#define VOLK_IMPLEMENTATION

#include <volk/volk.h>

#else
#include <vulkan/vulkan.h>
#endif /* USE_VOLK */

#define VMA_IMPLEMENTATION

#include <vma/vk_mem_alloc.h>

#ifdef USE_GLFW

#include <glfw/glfw3.h>

#endif /* USE_GLFW */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb/stb_image_write.h>

#include "utils/ioutils.h"

// std
#include <vector>
#include <algorithm>

void vrc_error_fatal(const char *msg, VkResult err)
{
    fprintf(stderr, "Error: %s (Result=%d)\n", msg, err);
    exit(EXIT_FAILURE);
}

void vrc_load_obj()
{
    /* TODO Implementation load obj model */
}

struct VrcDriver {
    uint32_t version = 0;
    VkInstance instance = VK_NULL_HANDLE;
#ifdef USE_GLFW
    VkSurfaceKHR surface = VK_NULL_HANDLE;
#endif /* USE_GLFW */
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    uint32_t queue_index = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
};

struct VrcSwapchainResourceEXT {
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
};

typedef struct VrcSwapchainEXT_T {
    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR capabilities = {};
    uint32_t min_image_count = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    std::vector<VrcSwapchainResourceEXT> resources;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t acquire_index = 0;
    uint32_t frame = 0;
    std::vector<VkSemaphore> acquire_index_semaphore;
    std::vector<VkSemaphore> render_finish_semaphore;
    std::vector<VkFence> fence;
    std::vector<VkCommandBuffer> command_buffers;
} *VrcSwapchainEXT;

typedef struct VrcBuffer_T {
    VkBuffer vk_buffer = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocation_info = {};
} *VrcBuffer;

typedef struct VrcPipeline_T {
    VkPipeline vk_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;
    VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
} *VrcPipeline;

typedef struct VrcTexture2D_T {
    VkImage vk_image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocation_info = {};
    VkImageView vk_view2d = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint32_t width = 0;
    uint32_t height = 0;
} *VrcTexture2D;

struct PushConstValue {
    glm::mat4 mvp;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

static Vertex vertices[] = {
    {{0.0f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
};

void vrc_swapchain_destroy(const VrcDriver *driver, VrcSwapchainEXT swapchain);

void vrc_pipeline_destroy(const VrcDriver *driver, VrcPipeline pipeline);

void vrc_texture2d_destroy(const VrcDriver *driver, VrcTexture2D texture);

void vrc_printf_device_limits(const VkPhysicalDevice &device)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

    VkDeviceSize max_vram_gb = 0;

    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            max_vram_gb = std::max(max_vram_gb, memoryProperties.memoryHeaps[i].size);
        }
    }

    max_vram_gb = max_vram_gb / (1024 * 1024 * 1024);

    printf("  |- maxVRAM(GB): %lluGB\n", max_vram_gb);

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

    printf("  |- maxPerStageDescriptorSamplers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorSamplers);
    printf("  |- maxDescriptorSetSamplers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetSamplers);
    printf("  |- maxDescriptorSetUniformBuffers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
    printf("  |- maxDescriptorSetUniformBuffers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
}

VkPhysicalDevice vrc_pick_discrete_device(const std::vector<VkPhysicalDevice> &devices)
{
    for (const auto &device: devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }

    assert(std::size(devices) > 0);
    return devices[0];
}

#ifdef USE_GLFW

VkResult vrc_pick_surface_format(const VrcDriver *driver, VkSurfaceFormatKHR *p_format)
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
        switch (item.format) {
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

#endif /* USE_GLFW */

void vrc_find_queue_index(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *p_index)
{
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, VK_NULL_HANDLE);

    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, std::data(properties));

    for (uint32_t i = 0; i < count; i++) {
        VkQueueFamilyProperties property = properties[i];
#ifdef USE_GLFW
        VkBool32 supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
        if ((property.queueFlags & VK_QUEUE_GRAPHICS_BIT) && supported) {
#else
            if ((property.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
#endif /* USE_GLFW */
            *p_index = i;
            return;
        }
    }

    vrc_error_fatal("Can't not found queue to support present", VK_ERROR_INITIALIZATION_FAILED);
}

VkResult vrc_load_shader_module(VkDevice device, const char *path, VkShaderModule *p_shader_module)
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

VkResult vrc_fence_create(const VrcDriver *driver, VkFence *p_fence)
{
    VkResult U_ASSERT_ONLY err;

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    return vkCreateFence(driver->device, &fenceCreateInfo, VK_NULL_HANDLE, p_fence);
}

void vrc_fence_destroy(const VrcDriver *driver, VkFence fence)
{
    vkDestroyFence(driver->device, fence, VK_NULL_HANDLE);
}

VkResult vrc_semaphore_create(const VrcDriver *driver, VkSemaphore *p_semaphore)
{
    VkResult err;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    return vkCreateSemaphore(driver->device, &semaphoreCreateInfo, VK_NULL_HANDLE, p_semaphore);
}

void vrc_semaphore_destroy(const VrcDriver *driver, VkSemaphore semaphore)
{
    vkDestroySemaphore(driver->device, semaphore, VK_NULL_HANDLE);
}

VkResult vrc_command_buffer_alloc(const VrcDriver *driver, VkCommandBuffer *p_command_buffer)
{
    VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = driver->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    return vkAllocateCommandBuffers(driver->device, &allocate_info, p_command_buffer);
}

void vrc_command_buffer_free(const VrcDriver *driver, VkCommandBuffer command_buffer)
{
    vkFreeCommandBuffers(driver->device, driver->command_pool, 1, &command_buffer);
}

VkResult vrc_buffer_create(const VrcDriver *driver, VkDeviceSize size, VrcBuffer *p_buffer, VkBufferUsageFlags usage)
{
    VrcBuffer tmp = memnew<VrcBuffer_T>();

    if (!tmp)
        return VK_ERROR_INITIALIZATION_FAILED;

    VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocation_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    tmp->size = size;
    *p_buffer = tmp;

    return vmaCreateBuffer(driver->allocator, &buffer_ci, &allocation_info, &tmp->vk_buffer, &tmp->allocation,
                           &tmp->allocation_info);
}

void vrc_buffer_destroy(VrcDriver *driver, VrcBuffer buffer)
{
    vmaDestroyBuffer(driver->allocator, buffer->vk_buffer, buffer->allocation);
    memdel(buffer);
}

VkResult vrc_image_view2d_create(const VrcDriver *driver, VkImage image, VkFormat format, VkImageView *p_view2d)
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

void vrc_image_view2d_destroy(const VrcDriver *driver, VkImageView view)
{
    vkDestroyImageView(driver->device, view, VK_NULL_HANDLE);
}

VkResult vrc_texture2d_create(const VrcDriver *driver, uint32_t w, uint32_t h, VrcTexture2D *p_texture)
{
    VkResult err;
    VrcTexture2D tmp = VK_NULL_HANDLE;

    tmp = memnew<VrcTexture2D_T>();

    if (!tmp)
        return VK_ERROR_INITIALIZATION_FAILED;

    tmp->width = w;
    tmp->height = h;
    tmp->format = VK_FORMAT_R8G8B8A8_SRGB;

    auto texture2d_cleanup = [&](VkResult err) {
        if (err) {
            vrc_texture2d_destroy(driver, tmp);
        }
        return err;
    };

    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = tmp->format,
        .extent = {w, h, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    if ((err = vmaCreateImage(driver->allocator, &imageCreateInfo, &allocationCreateInfo, &tmp->vk_image,
                              &tmp->allocation, &tmp->allocation_info)))
        return texture2d_cleanup(err);

    if ((err = vrc_image_view2d_create(driver, tmp->vk_image, tmp->format, &tmp->vk_view2d)))
        return texture2d_cleanup(err);

    *p_texture = tmp;

    return err;
}

void vrc_texture2d_destroy(const VrcDriver *driver, VrcTexture2D texture)
{
    if (!texture)
        return;

    if (texture->vk_view2d != VK_NULL_HANDLE)
        vkDestroyImageView(driver->device, texture->vk_view2d, VK_NULL_HANDLE);

    if (texture->vk_image != VK_NULL_HANDLE)
        vmaDestroyImage(driver->allocator, texture->vk_image, texture->allocation);

    memdel(texture);
}

VkResult
vrc_descriptor_set_layout_create(const VrcDriver *driver, uint32_t bind_count, VkDescriptorSetLayoutBinding *p_binds, VkDescriptorSetLayout *p_layout)
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bind_count,
        .pBindings = p_binds,
    };

    return vkCreateDescriptorSetLayout(driver->device, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, p_layout);
}

VkResult
vrc_descriptor_set_alloc(const VrcDriver *driver, VkDescriptorSetLayout layout, VkDescriptorSet *p_descriptor_set)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = driver->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    return vkAllocateDescriptorSets(driver->device, &descriptorSetAllocateInfo, p_descriptor_set);
}

VkResult
vrc_descriptor_set_free(const VrcDriver *driver, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set)
{
    return vkFreeDescriptorSets(driver->device, descriptor_pool, 1, &descriptor_set);
}

VkResult vrc_pipeline_create(const VrcDriver *driver, VkFormat color, VrcPipeline *p_pipeline)
{
    VkResult err;
    VrcPipeline tmp = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_module = VK_NULL_HANDLE;

    tmp = memnew<VrcPipeline_T>();

    if (!tmp)
        return VK_ERROR_INITIALIZATION_FAILED;

    /* use cleanup to clear resource when some error. */
    auto vrc_pipeline_cleanup = [&](VkResult err) {
        if (vertex_shader_module != VK_NULL_HANDLE)
            vkDestroyShaderModule(driver->device, vertex_shader_module, VK_NULL_HANDLE);

        if (fragment_shader_module != VK_NULL_HANDLE)
            vkDestroyShaderModule(driver->device, fragment_shader_module, VK_NULL_HANDLE);

        if (err != VK_SUCCESS)
            vrc_pipeline_destroy(driver, tmp);

        return err;
    };

    if ((err = vrc_load_shader_module(driver->device, "shaders/spir-v/simple_vertex.spv", &vertex_shader_module)))
        return vrc_pipeline_cleanup(err);

    if ((err = vrc_load_shader_module(driver->device, "shaders/spir-v/simple_fragment.spv", &fragment_shader_module)))
        return vrc_pipeline_cleanup(err);

    VkPushConstantRange pushConstantRanges[] = {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(PushConstValue),
        }
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = std::size(pushConstantRanges),
        .pPushConstantRanges = std::data(pushConstantRanges),
    };

    if ((err = vkCreatePipelineLayout(driver->device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE,
                                      &tmp->vk_pipeline_layout)))
        return vrc_pipeline_cleanup(err);

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module,
            .pName = "main",
        }
    };

    VkVertexInputBindingDescription vertexInputBindingDescriptions[] = {
        {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, color)
        }
    };

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = std::size(vertexInputBindingDescriptions),
        .pVertexBindingDescriptions = std::data(vertexInputBindingDescriptions),
        .vertexAttributeDescriptionCount = std::size(vertexInputAttributeDescriptions),
        .pVertexAttributeDescriptions = std::data(vertexInputAttributeDescriptions),
    };

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    //     .depthTestEnable = VK_TRUE,
    //     .depthWriteEnable = VK_TRUE,
    //     .depthCompareOp = VK_COMPARE_OP_LESS,
    // };

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &pipelineColorBlendAttachmentState,
    };

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &color,
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = std::size(dynamicStates),
        .pDynamicStates = std::data(dynamicStates),
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = std::size(pipelineShaderStageCreateInfo),
        .pStages = std::data(pipelineShaderStageCreateInfo),
        .pVertexInputState = &pipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
        .pViewportState = &pipelineViewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = &pipelineMultisampleStateCreateInfo,
        // .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &pipelineColorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = tmp->vk_pipeline_layout,
    };

    if ((err = vkCreateGraphicsPipelines(driver->device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo,
                                         VK_NULL_HANDLE, &tmp->vk_pipeline)))
        return vrc_pipeline_cleanup(err);

    *p_pipeline = tmp;

    return vrc_pipeline_cleanup(err);
}

void vrc_pipeline_destroy(const VrcDriver *driver, VrcPipeline pipeline)
{
    if (!pipeline)
        return;

    if (pipeline->vk_pipeline_layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(driver->device, pipeline->vk_pipeline_layout, VK_NULL_HANDLE);

    if (pipeline->vk_pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(driver->device, pipeline->vk_pipeline, VK_NULL_HANDLE);

    memdel(pipeline);
}

void vrc_memory_read(const VrcDriver *driver, VrcBuffer buffer, size_t size, void *dst)
{
    void *src;
    vmaMapMemory(driver->allocator, buffer->allocation, &src);
    memcpy(dst, src, size);
    vmaUnmapMemory(driver->allocator, buffer->allocation);
}

void vrc_memory_write(const VrcDriver *driver, VrcBuffer buffer, size_t size, void *src)
{
    void *dst;
    vmaMapMemory(driver->allocator, buffer->allocation, &dst);
    memcpy(dst, src, size);
    vmaUnmapMemory(driver->allocator, buffer->allocation);
}

void vrc_memory_image_barrier(VkCommandBuffer command_buffer,
                              VkImage image,
                              VkAccessFlags srcAccessMask,
                              VkAccessFlags dstAccessMask,
                              VkImageLayout oldLayout,
                              VkImageLayout newLayout,
                              VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask)
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
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresource,
    };

    vkCmdPipelineBarrier(
        command_buffer,
        srcStageMask,
        dstStageMask,
        0,
        0,
        VK_NULL_HANDLE,
        0,
        VK_NULL_HANDLE,
        1,
        &barrier);
}

VkResult vrc_acquire_next_index(const VrcDriver *driver, VrcSwapchainEXT swapchain, uint32_t *p_index)
{
    return vkAcquireNextImageKHR(driver->device,
                                 swapchain->vk_swapchain,
                                 UINT64_MAX,
                                 swapchain->acquire_index_semaphore[swapchain->frame],
                                 VK_NULL_HANDLE,
                                 p_index);
}

void vrc_cmd_begin(VkCommandBuffer command_buffer)
{
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                 VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void vrc_cmd_end(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);
}

void vrc_cmd_begin_once(VkCommandBuffer command_buffer)
{
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void vrc_cmd_end_once(VkCommandBuffer command_buffer)
{
    vrc_cmd_end(command_buffer);
}

void vrc_cmd_begin_rendering(VkCommandBuffer command_buffer, uint32_t w, uint32_t h, VkImageView view2d)
{
    vrc_cmd_begin(command_buffer);

    VkRenderingAttachmentInfo renderingAttachmentInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = view2d,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {
            .color = {0.0f, 0.0f, 0.0f, 1.0f},
        },
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.offset = {0, 0},
            .extent = {w, h}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &renderingAttachmentInfo,
        .pDepthAttachment = VK_NULL_HANDLE,
    };

    vkCmdBeginRendering(command_buffer, &renderingInfo);

    VkViewport viewport = {
        .width = static_cast<float>(w),
        .height = static_cast<float>(h),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {w, h}
    };

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void vrc_cmd_end_rendering(VkCommandBuffer command_buffer)
{
    vkCmdEndRendering(command_buffer);
    vrc_cmd_end(command_buffer);
}

void vrc_cmd_bind_pipeline(VkCommandBuffer command_buffer, VrcPipeline pipeline)
{
    vkCmdBindPipeline(command_buffer, pipeline->bindpoint, pipeline->vk_pipeline);
}

void
vrc_cmd_push_constants(VkCommandBuffer command_buffer, VrcPipeline pipeline, VkShaderStageFlags stage, uint32_t offset, size_t size, const void *ptr)
{
    vkCmdPushConstants(command_buffer, pipeline->vk_pipeline_layout, stage, offset, size, ptr);
}

void vrc_cmd_bind_vertex_buffer(VkCommandBuffer command_buffer, VrcBuffer vertex_buffer)
{
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer->vk_buffer, offsets);
}

void vrc_cmd_draw(VkCommandBuffer command_buffer, uint32_t vertex_count)
{
    vkCmdDraw(command_buffer, vertex_count, 1, 0, 0);
}

uint32_t vrc_cmd_copy_image(VkCommandBuffer command_buffer, VrcTexture2D src, VrcBuffer dst)
{
    vrc_memory_image_barrier(command_buffer,
                             src->vk_image,
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region = {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = {src->width, src->height, 1},
    };

    vkCmdCopyImageToBuffer(command_buffer,
                           src->vk_image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           dst->vk_buffer,
                           1,
                           &region);

    return (src->width * src->height);
}

VkResult vrc_queue_submit(const VrcDriver *driver,
                          VkCommandBuffer command_buffer,
                          uint32_t wait_count,
                          VkSemaphore *p_waits,
                          VkPipelineStageFlags *p_stage_mask,
                          uint32_t signal_count,
                          VkSemaphore *p_signals,
                          VkFence fence)
{
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = wait_count,
        .pWaitSemaphores = p_waits,
        .pWaitDstStageMask = p_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = signal_count,
        .pSignalSemaphores = p_signals,
    };

    return vkQueueSubmit(driver->queue, 1, &submitInfo, fence);
}

VkResult vrc_present_submit(const VrcDriver *driver, VkCommandBuffer command_buffer, VrcSwapchainEXT swapchain)
{
    VkPipelineStageFlags wait_pipeline_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vrc_queue_submit(driver,
                     command_buffer,
                     1,
                     &swapchain->acquire_index_semaphore[swapchain->frame],
                     &wait_pipeline_stage,
                     1,
                     &swapchain->render_finish_semaphore[swapchain->frame],
                     swapchain->fence[swapchain->frame]);

    VkCommandBuffer once_command_buffer;
    vrc_command_buffer_alloc(driver, &once_command_buffer);

    vrc_cmd_begin_once(once_command_buffer);

    vrc_memory_image_barrier(once_command_buffer,
                             swapchain->resources[swapchain->acquire_index].image,
                             VK_ACCESS_NONE,
                             VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    vrc_cmd_end_once(once_command_buffer);
    vrc_queue_submit(driver, once_command_buffer, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain->render_finish_semaphore[swapchain->frame],
        .swapchainCount = 1,
        .pSwapchains = &swapchain->vk_swapchain,
        .pImageIndices = &swapchain->acquire_index,
    };

    return vkQueuePresentKHR(driver->queue, &presentInfo);
}

VkResult vrc_device_wait_idle(const VrcDriver *driver)
{
    return vkDeviceWaitIdle(driver->device);
}

VkResult vrc_queue_wait_idle(const VrcDriver *driver)
{
    return vkQueueWaitIdle(driver->queue);
}

#ifdef USE_GLFW

VrcDriver *vrc_driver_init(GLFWwindow *window)
#else

VrcDriver *vrc_driver_init()
#endif /* USE_GLFW */
{
    VkResult err;
    uint32_t count;

    VrcDriver *driver = memnew<VrcDriver>();

    if (!driver)
        vrc_error_fatal("Failed to initialize driver struct, cause: memory allocate failed",
                        VK_ERROR_INITIALIZATION_FAILED);

#ifdef USE_VOLK
    /*
     * initialize volk loader to dynamic load about instance function
     * api pointer.
     */
    if ((err = volkInitialize()))
        vrc_error_fatal("Failed to initialize volk loader", err);
#endif /* USE_VOLK */

    if ((err = vkEnumerateInstanceVersion(&driver->version)))
        vrc_error_fatal("Can't not get instance version", err);

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

    std::vector<const char *> extensions;

#ifdef USE_GLFW
    const char **required = glfwGetRequiredInstanceExtensions(&count);
    for (int i = 0; i < count; ++i)
        extensions.push_back(required[i]);
#endif /* USE_GLFW */

    std::vector<const char *> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instance_ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &info,
        .enabledLayerCount = (uint32_t) std::size(layers),
        .ppEnabledLayerNames = std::data(layers),

#ifdef USE_GLFW
        .enabledExtensionCount = (uint32_t) std::size(extensions),
        .ppEnabledExtensionNames = std::data(extensions),
#endif /* USE_GLFW */

    };

    if ((err = vkCreateInstance(&instance_ci, VK_NULL_HANDLE, &driver->instance)))
        vrc_error_fatal("Failed to create instance", err);

#ifdef USE_VOLK
    volkLoadInstance(driver->instance);
#endif /* USE_VOLK */

    if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, VK_NULL_HANDLE)))
        vrc_error_fatal("Failed to enumerate physical device list count", err);

    std::vector<VkPhysicalDevice> devices(count);
    if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, std::data(devices))))
        vrc_error_fatal("Failed to enumerate physical device list data", err);

    driver->gpu = vrc_pick_discrete_device(devices);

#ifdef USE_GLFW
    if ((err = glfwCreateWindowSurface(driver->instance, window, VK_NULL_HANDLE, &driver->surface)))
        vrc_error_fatal("Failed to create surface", err);
#endif /* USE_GLFW */

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(driver->gpu, &properties);
    printf("Use GPU %s\n", properties.deviceName);

    vrc_printf_device_limits(driver->gpu);

    float priorities = 1.0f;

#ifdef USE_GLFW
    vrc_find_queue_index(driver->gpu, driver->surface, &driver->queue_index);
#else
    vrc_find_queue_index(driver->gpu, VK_NULL_HANDLE, &driver->queue_index);
#endif /* USE_GLFW */

    VkDeviceQueueCreateInfo queue_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = driver->queue_index,
        .queueCount = 1,
        .pQueuePriorities = &priorities,
    };

    std::vector<const char *> device_extensions;

#ifdef USE_GLFW
    device_extensions.push_back("VK_KHR_swapchain");
#endif /* USE_GLFW */

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceCreateInfo device_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRenderingFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_ci,
        .enabledExtensionCount = (uint32_t) std::size(device_extensions),
        .ppEnabledExtensionNames = std::data(device_extensions),
    };

    if ((err = vkCreateDevice(driver->gpu, &device_ci, VK_NULL_HANDLE, &driver->device)))
        vrc_error_fatal("Failed to create logic device", err);

#ifdef USE_VOLK
    volkLoadDevice(driver->device);
#endif /* USE_VOLK */

    vkGetDeviceQueue(driver->device, driver->queue_index, 0, &driver->queue);

    VkCommandPoolCreateInfo command_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = driver->queue_index,
    };

    if ((err = vkCreateCommandPool(driver->device, &command_pool_ci, VK_NULL_HANDLE, &driver->command_pool)))
        vrc_error_fatal("Failed to create command pool", err);

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
        vrc_error_fatal("Failed to create VMA allocator", err);

    VkDescriptorPoolSize descriptorPoolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256},
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 512,
        .poolSizeCount = std::size(descriptorPoolSizes),
        .pPoolSizes = std::data(descriptorPoolSizes),
    };

    if ((err = vkCreateDescriptorPool(driver->device, &descriptorPoolCreateInfo, VK_NULL_HANDLE,
                                      &driver->descriptor_pool)))
        vrc_error_fatal("Failed to create descriptor pool", err);

    return driver;
}

void vrc_driver_destroy(VrcDriver *driver)
{
    vkDeviceWaitIdle(driver->device);
    vkDestroyDescriptorPool(driver->device, driver->descriptor_pool, VK_NULL_HANDLE);
    vmaDestroyAllocator(driver->allocator);
    vkDestroyCommandPool(driver->device, driver->command_pool, VK_NULL_HANDLE);
    vkDestroyDevice(driver->device, VK_NULL_HANDLE);

#ifdef USE_GLFW
    vkDestroySurfaceKHR(driver->instance, driver->surface, VK_NULL_HANDLE);
#endif /* USE_GLFW */

    vkDestroyInstance(driver->instance, VK_NULL_HANDLE);
    memdel(driver);
}

#ifdef USE_GLFW

VkResult
vrc_swapchain_create(const VrcDriver *driver, VrcSwapchainEXT *p_swapchain, VrcSwapchainEXT exist = VK_NULL_HANDLE)
{
    VkResult err;
    VrcSwapchainEXT tmp = VK_NULL_HANDLE;
    std::vector<VkImage> images;

    tmp = memnew<VrcSwapchainEXT_T>();

    if (!tmp)
        return VK_ERROR_INITIALIZATION_FAILED;

    /* use cleanup to clear resource when some error. */
    auto vrc_swapchain_cleanup = [&](VkResult err) {
        vrc_swapchain_destroy(driver, tmp);
        return err;
    };

    if ((err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(driver->gpu, driver->surface, &tmp->capabilities)))
        return vrc_swapchain_cleanup(err);

    uint32_t min = tmp->capabilities.minImageCount;
    uint32_t max = tmp->capabilities.maxImageCount;
    tmp->min_image_count = std::clamp(min + 1, min, max);

    tmp->width = tmp->capabilities.currentExtent.width;
    tmp->height = tmp->capabilities.currentExtent.height;

    *p_swapchain = tmp;

    VkSurfaceFormatKHR surface_format;
    if ((err = vrc_pick_surface_format(driver, &surface_format)))
        return vrc_swapchain_cleanup(err);

    tmp->format = surface_format.format;
    tmp->color_space = surface_format.colorSpace;

    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = driver->surface,
        .minImageCount = tmp->min_image_count,
        .imageFormat = tmp->format,
        .imageColorSpace = tmp->color_space,
        .imageExtent = tmp->capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = tmp->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = exist ? exist->vk_swapchain : VK_NULL_HANDLE,
    };

    if ((err = vkCreateSwapchainKHR(driver->device, &swapchainCreateInfoKHR, VK_NULL_HANDLE, &tmp->vk_swapchain)))
        return vrc_swapchain_cleanup(err);

    uint32_t count;
    if ((err = vkGetSwapchainImagesKHR(driver->device, tmp->vk_swapchain, &count, VK_NULL_HANDLE)))
        return vrc_swapchain_cleanup(err);

    images.resize(count);
    if ((err = vkGetSwapchainImagesKHR(driver->device, tmp->vk_swapchain, &count, std::data(images))))
        return vrc_swapchain_cleanup(err);

    tmp->resources.resize(tmp->min_image_count);
    tmp->acquire_index_semaphore.resize(tmp->min_image_count);
    tmp->render_finish_semaphore.resize(tmp->min_image_count);
    tmp->fence.resize(tmp->min_image_count);
    tmp->command_buffers.resize(tmp->min_image_count);

    for (int i = 0; i < tmp->min_image_count; ++i) {

        tmp->resources[i].image = images[i];
        if ((err = vrc_image_view2d_create(driver, tmp->resources[i].image, tmp->format,
                                           &(tmp->resources[i].image_view))))
            return vrc_swapchain_cleanup(err);

        if ((err = vrc_semaphore_create(driver, &(tmp->acquire_index_semaphore[i]))))
            return vrc_swapchain_cleanup(err);

        if ((err = vrc_semaphore_create(driver, &(tmp->render_finish_semaphore[i]))))
            return vrc_swapchain_cleanup(err);

        if ((err = vrc_fence_create(driver, &(tmp->fence[i]))))
            return vrc_swapchain_cleanup(err);

        vrc_command_buffer_alloc(driver, &(tmp->command_buffers[i]));
    }

    return err;
}

void vrc_swapchain_resize_check(const VrcDriver *driver, VrcSwapchainEXT *p_swapchain)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(driver->gpu, driver->surface, &capabilities);

    VkExtent2D extent = capabilities.currentExtent;
    if ((extent.width != 0 || extent.height != 0) &&
        ((*p_swapchain)->width != extent.width || (*p_swapchain)->height != extent.height)) {
        vkDeviceWaitIdle(driver->device);
        VrcSwapchainEXT tmp = VK_NULL_HANDLE;
        vrc_swapchain_create(driver, &tmp, (*p_swapchain));
        vrc_swapchain_destroy(driver, (*p_swapchain));
        *p_swapchain = tmp;
    }
}

void vrc_swapchain_destroy(const VrcDriver *driver, VrcSwapchainEXT swapchain)
{
    if (swapchain == VK_NULL_HANDLE)
        return;

    vkQueueWaitIdle(driver->queue);

    for (int i = 0; i < swapchain->min_image_count; ++i) {
        auto resource = swapchain->resources[i];

        if (resource.image_view != VK_NULL_HANDLE)
            vrc_image_view2d_destroy(driver, resource.image_view);

        if (swapchain->acquire_index_semaphore[i] != VK_NULL_HANDLE)
            vrc_semaphore_destroy(driver, swapchain->acquire_index_semaphore[i]);

        if (swapchain->render_finish_semaphore[i] != VK_NULL_HANDLE)
            vrc_semaphore_destroy(driver, swapchain->render_finish_semaphore[i]);

        if (swapchain->fence[i] != VK_NULL_HANDLE)
            vrc_fence_destroy(driver, swapchain->fence[i]);

        if (swapchain->command_buffers[i] != VK_NULL_HANDLE)
            vrc_command_buffer_free(driver, swapchain->command_buffers[i]);
    }

    vkDestroySwapchainKHR(driver->device, swapchain->vk_swapchain, VK_NULL_HANDLE);
    memdel(swapchain);
}

#endif /* USE_GLFW */

int main()
{
    VkResult err;

    system("chcp 65001 >nul");

    /*
     * close stdout and stderr write to buf, let direct
     * output.
     */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    /* copy spir-v shader binary files */
    system("cd ../shaders && shaderc");


#ifdef USE_GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = VK_NULL_HANDLE;
#endif /* USE_GLFW */

    VrcDriver *driver = VK_NULL_HANDLE;

#ifdef USE_GLFW
    VrcSwapchainEXT swapchain = VK_NULL_HANDLE;
#endif /* USE_GLFW */

    VrcBuffer vertex_buffer = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ring = VK_NULL_HANDLE;
    VrcPipeline pipeline = VK_NULL_HANDLE;

#ifndef USE_GLFW
    VrcTexture2D texture = VK_NULL_HANDLE;
#endif /* USE_GLFW */

#ifdef USE_GLFW
    window = glfwCreateWindow(800, 600, "Vronk Cube", nullptr, nullptr);
    driver = vrc_driver_init(window);
    vrc_swapchain_create(driver, &swapchain);
#else
    driver = vrc_driver_init();

    vrc_command_buffer_alloc(driver, &command_buffer_ring);
#endif /* USE_GLFW */

    // create vertex buffer
    if ((err = vrc_buffer_create(driver, sizeof(vertices), &vertex_buffer,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)))
        vrc_error_fatal("Failed to create vertex buffer", err);

    vrc_memory_write(driver, vertex_buffer, sizeof(vertices), (void *) vertices);

#ifdef USE_GLFW
    err = vrc_pipeline_create(driver, swapchain->format, &pipeline);
#else
    err = vrc_pipeline_create(driver, VK_FORMAT_R8G8B8A8_SRGB, &pipeline);
#endif /* USE_GLFW */

    if (err) vrc_error_fatal("Failed to create graphics pipeline", err);

#ifndef USE_GLFW
    if ((err = vrc_texture2d_create(driver, 800, 600, &texture)))
        vrc_error_fatal("Failed to create texture 2d", err);
#endif /* USE_GLFW */

    glm::mat4 model(1.0f);
    glm::mat4 view(1.0f);
    glm::mat4 proj(1.0f);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0));

    view = glm::lookAt(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0, 0.0f));

#ifdef USE_GLFW
    while (!glfwWindowShouldClose(window)) {
        vrc_swapchain_resize_check(driver, &swapchain);

        swapchain->frame = (swapchain->frame + 1) % swapchain->min_image_count;
        vrc_acquire_next_index(driver, swapchain, &swapchain->acquire_index);

        command_buffer_ring = swapchain->command_buffers[swapchain->frame];
        VkImageView view2d = swapchain->resources[swapchain->acquire_index].image_view;
        vrc_cmd_begin_rendering(command_buffer_ring, swapchain->width, swapchain->height, view2d);
        float aspect = (float) swapchain->width / (float) swapchain->height;
#else
        float aspect = 800 / 600;
        vrc_cmd_begin_rendering(command_buffer_ring, texture->width, texture->height, texture->vk_view2d);
#endif /* USE_GLFW */

        proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        proj[1][1] *= -1;

        PushConstValue push_const;
        push_const.mvp = proj * view * model;

        vrc_cmd_bind_pipeline(command_buffer_ring, pipeline);
        vrc_cmd_push_constants(command_buffer_ring, pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstValue),
                               &push_const);
        vrc_cmd_bind_vertex_buffer(command_buffer_ring, vertex_buffer);
        vrc_cmd_draw(command_buffer_ring, sizeof(vertices));
        vrc_cmd_end_rendering(command_buffer_ring);

#ifdef USE_GLFW
        vrc_present_submit(driver, command_buffer_ring, swapchain);

        vkWaitForFences(driver->device, 1, &(swapchain->fence[swapchain->frame]), VK_TRUE, UINT64_MAX);
        vkResetFences(driver->device, 1, &(swapchain->fence[swapchain->frame]));
        vkResetCommandBuffer(command_buffer_ring, 0);
#else
        vrc_queue_submit(driver, command_buffer_ring, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, VK_NULL_HANDLE);
#endif /* USE_GLFW */

#ifndef USE_GLFW
        vrc_queue_wait_idle(driver);
        VrcBuffer copy_buffer;
        uint32_t copy_size = texture->width * texture->height * 4;
    
        if ((err = vrc_buffer_create(driver, copy_size, &copy_buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT)))
            vrc_error_fatal("Failed to create copy_buffer", err);
    
        VkCommandBuffer copy_command_buffer;
        vrc_command_buffer_alloc(driver, &copy_command_buffer);
    
        vrc_cmd_begin(copy_command_buffer);
        vrc_cmd_copy_image(copy_command_buffer, texture, copy_buffer);
        vrc_cmd_end(copy_command_buffer);
        vrc_queue_submit(driver, copy_command_buffer, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0);
        vrc_queue_wait_idle(driver);
    
        vrc_command_buffer_free(driver, copy_command_buffer);
    
        void *data = malloc(copy_size);
        vrc_memory_read(driver, copy_buffer, copy_size, data);
        stbi_write_png("./rendered.png", texture->width, texture->height, 4, data, texture->width * 4);
        free(data);
    
        vrc_buffer_destroy(driver, copy_buffer);
#endif /* USE_GLFW */

#ifdef USE_GLFW
        glfwPollEvents();
    }
#endif /* USE_GLFW */

#ifndef USE_GLFW
    vrc_texture2d_destroy(driver, texture);
    vrc_command_buffer_free(driver, command_buffer_ring);
#endif /* USE_GLFW */

    vrc_pipeline_destroy(driver, pipeline);
    vrc_buffer_destroy(driver, vertex_buffer);

#ifdef USE_GLFW
    vrc_swapchain_destroy(driver, swapchain);
#endif /* USE_GLFW */

    vrc_driver_destroy(driver);

#ifdef USE_GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
#endif /* USE_GLFW */

    return 0;
}
