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

#ifdef USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#else
#include <vulkan/vulkan.h>
#endif /* USE_VOLK */

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <ImGuizmo/ImGuizmo.h>
#include <imnodes/imnodes.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "utils/ioutils.h"

// std
#include <vector>
#include <algorithm>
#include <unordered_map>

void vrc_error_fatal(const char *msg, VkResult err)
{
    fprintf(stderr, "Error: %s (Result=%d)\n", msg, err);
    exit(EXIT_FAILURE);
}

struct VrcDriver {
    uint32_t version = 0;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
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
    float aspect = 0.0f;
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
    VkDescriptorSetLayout vk_descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorSet vk_sampler2d_descriptor = VK_NULL_HANDLE;
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
    VkSampler sampler = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
} *VrcTexture2D;

struct PushConstValue {
    glm::mat4 mvp;
};

struct vertex_t {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
    
    bool operator==(const vertex_t & other) const {
        return this->position == other.position && this->texcoord == other.texcoord && this->normal == other.normal;
    }
};

namespace std {
    template <> struct hash<vertex_t> {
        size_t operator()(const vertex_t & vertex) const {
            std::size_t h1 = std::hash<float>{}(vertex.position.x);
            std::size_t h2 = std::hash<float>{}(vertex.position.y);
            std::size_t h3 = std::hash<float>{}(vertex.position.z);
            std::size_t h4 = std::hash<float>{}(vertex.texcoord.x);
            std::size_t h5 = std::hash<float>{}(vertex.texcoord.y);
            std::size_t h6 = std::hash<float>{}(vertex.normal.x);
            std::size_t h7 = std::hash<float>{}(vertex.normal.y);
            std::size_t h8 = std::hash<float>{}(vertex.normal.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6) ^ (h8 << 7);
        }
    };
}

struct Link {
    int id;
    int start_attr;
    int end_attr;
};

std::vector<Link> editor_links;

void vrc_swapchain_destroy(const VrcDriver *driver, VrcSwapchainEXT swapchain);
void vrc_pipeline_destroy(const VrcDriver *driver, VrcPipeline pipeline);
void vrc_texture2d_destroy(const VrcDriver *driver, VrcTexture2D texture);
void vrc_cmd_begin_once(VkCommandBuffer command_buffer);
void vrc_cmd_end_once(VkCommandBuffer command_buffer);
VkResult vrc_queue_submit(const VrcDriver *driver,
                          VkCommandBuffer command_buffer,
                          uint32_t wait_count,
                          VkSemaphore *p_waits,
                          VkPipelineStageFlags *p_stage_mask,
                          uint32_t signal_count,
                          VkSemaphore *p_signals,
                          VkFence fence);
void vrc_memory_image_barrier(VkCommandBuffer command_buffer,
                              VkImage image,
                              VkAccessFlags srcAccessMask,
                              VkAccessFlags dstAccessMask,
                              VkImageLayout oldLayout,
                              VkImageLayout newLayout,
                              VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask);
                              
void vrc_load_obj_model(const char *path, std::vector<vertex_t>& vertices, std::vector<uint32_t>& indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        if (!warn.empty())
            printf("Wran: %s\n", warn.c_str());
        if (!err.empty())
            printf("Error: %s\n", err.c_str());
        vrc_error_fatal("Failed to load obj", VK_ERROR_INITIALIZATION_FAILED);
    }

    std::unordered_map<vertex_t, uint32_t> unique_map;
    
    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            vertex_t vertex = {};
            
            if (idx.vertex_index >= 0) {
                vertex.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.position[2] = attrib.vertices[3 * idx.vertex_index + 2];
            }
            
            if (idx.texcoord_index >= 0) {
                vertex.texcoord[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                vertex.texcoord[1] = attrib.texcoords[2 * idx.texcoord_index + 1];
            }
            
            if (idx.normal_index >= 0) {
                vertex.normal[0] = attrib.normals[3 * idx.normal_index + 0];
                vertex.normal[1] = attrib.normals[3 * idx.normal_index + 1];
                vertex.normal[2] = attrib.normals[3 * idx.normal_index + 2];
            }
            
            if (unique_map.count(vertex) == 0) {
                unique_map[vertex] = static_cast<uint32_t>(std::size(vertices));
                vertices.push_back(vertex);
            }
            
            indices.push_back(unique_map[vertex]);
        }
    }
    
}

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

void vrc_find_queue_index(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *p_index)
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

    return vmaCreateBuffer(driver->allocator, &buffer_ci, &allocation_info, &tmp->vk_buffer, &tmp->allocation, &tmp->allocation_info);
}

void vrc_buffer_destroy(const VrcDriver *driver, VrcBuffer buffer)
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

VkResult vrc_sampler_create(const VrcDriver *driver, VkSampler *p_sampler)
{
    VkSamplerCreateInfo samplerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .maxAnisotropy = 16,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    };

    return vkCreateSampler(driver->device, &samplerCreateInfo, VK_NULL_HANDLE, p_sampler);
}

void vrc_sampler_destroy(const VrcDriver *driver, VkSampler sampler)
{
    vkDestroySampler(driver->device, sampler, VK_NULL_HANDLE);
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
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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

    if ((err = vrc_sampler_create(driver, &tmp->sampler)))
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

    if (texture->sampler != VK_NULL_HANDLE)
        vkDestroySampler(driver->device, texture->sampler, VK_NULL_HANDLE);

    memdel(texture);
}

VkResult vrc_descriptor_set_layout_create(const VrcDriver *driver, uint32_t bind_count, VkDescriptorSetLayoutBinding *p_binds, VkDescriptorSetLayout *p_layout)
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bind_count,
        .pBindings = p_binds,
    };

    return vkCreateDescriptorSetLayout(driver->device, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, p_layout);
}

void vrc_descriptor_set_layout_destroy(const VrcDriver* driver, VkDescriptorSetLayout descriptorSetLayout)
{
    vkDestroyDescriptorSetLayout(driver->device, descriptorSetLayout, VK_NULL_HANDLE);
}

VkResult vrc_descriptor_set_alloc(const VrcDriver *driver, VkDescriptorSetLayout layout, VkDescriptorSet *p_descriptor_set)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = driver->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    return vkAllocateDescriptorSets(driver->device, &descriptorSetAllocateInfo, p_descriptor_set);
}

VkResult vrc_descriptor_set_free(const VrcDriver *driver, VkDescriptorSet descriptor_set)
{
    return vkFreeDescriptorSets(driver->device, driver->descriptor_pool, 1, &descriptor_set);
}

void vrc_descriptor_set_write(const VrcDriver* driver, VkDescriptorSet descriptorSet, VrcTexture2D texture)
{
    VkDescriptorImageInfo imageInfo = {
        .sampler = texture->sampler,
        .imageView = texture->vk_view2d,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(driver->device, 1, &descriptorWrite, 0, nullptr);
}

void vrc_cmd_bind_descriptor_set(VkCommandBuffer command_buffer, VrcPipeline pipeline, VkDescriptorSet descriptorSet)
{
    vkCmdBindDescriptorSets(command_buffer, pipeline->bindpoint, pipeline->vk_pipeline_layout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
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

    // 创建 DescriptorSet
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[] = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE }
    };
    
    if ((err = vrc_descriptor_set_layout_create(driver, std::size(descriptorSetLayoutBinding), std::data(descriptorSetLayoutBinding), &tmp->vk_descriptor_set_layout)))
        return vrc_pipeline_cleanup(err);

    if ((err = vrc_descriptor_set_alloc(driver, tmp->vk_descriptor_set_layout, &tmp->vk_sampler2d_descriptor)))
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
        .setLayoutCount = 1,
        .pSetLayouts = &tmp->vk_descriptor_set_layout,
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
            .stride = sizeof(vertex_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex_t, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(vertex_t, texcoord)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex_t, normal)
        },
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
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
    };

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
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
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

    if (pipeline->vk_sampler2d_descriptor != VK_NULL_HANDLE)
        vrc_descriptor_set_free(driver, pipeline->vk_sampler2d_descriptor);

    if (pipeline->vk_descriptor_set_layout != VK_NULL_HANDLE)
        vrc_descriptor_set_layout_destroy(driver, pipeline->vk_descriptor_set_layout);

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

VkResult vrc_memory_write(const VrcDriver* driver, VrcTexture2D texture, size_t size, void* data)
{
    VkResult U_ASSERT_ONLY err;
    VrcBuffer tmp;

    vrc_buffer_create(driver, size, &tmp, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    
    vrc_memory_write(driver, tmp, size, data);
    
    VkCommandBuffer command_buffer;
    vrc_command_buffer_alloc(driver, &command_buffer);
    vrc_cmd_begin_once(command_buffer);

    vrc_memory_image_barrier(command_buffer,
                             texture->vk_image,
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);
    
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {0, 0},
        .imageExtent = {
            .width = texture->width,
            .height = texture->height,
            .depth = 1
        }
    };
    
    vkCmdCopyBufferToImage(command_buffer,
                           tmp->vk_buffer,
                           texture->vk_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);

    vrc_memory_image_barrier(command_buffer,
                             texture->vk_image,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    
    vrc_cmd_end_once(command_buffer);

    VkFence fence;
    vrc_fence_create(driver, &fence);
    vrc_queue_submit(driver, command_buffer, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, fence);
    vkWaitForFences(driver->device, 1, &fence, VK_TRUE, UINT64_MAX);
    vrc_fence_destroy(driver, fence);

    vrc_command_buffer_free(driver, command_buffer);
    vrc_buffer_destroy(driver, tmp);
    
    return err;
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
        .renderArea = {.offset = {0, 0}, .extent = {w, h}},
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

void vrc_cmd_bind_index_buffer(VkCommandBuffer command_buffer, VrcBuffer index_buffer)
{
    vkCmdBindIndexBuffer(command_buffer, index_buffer->vk_buffer, 0, VK_INDEX_TYPE_UINT32);
}

void vrc_cmd_draw(VkCommandBuffer command_buffer, uint32_t vertex_count)
{
    vkCmdDraw(command_buffer, vertex_count, 1, 0, 0);
}

void vrc_cmd_draw_indexed(VkCommandBuffer command_buffer, uint32_t index_count)
{
    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
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

VrcDriver *vrc_driver_init(GLFWwindow *window)
{
    VkResult err;
    uint32_t count;

    VrcDriver *driver = memnew<VrcDriver>();

    if (!driver)
        vrc_error_fatal("Failed to initialize driver struct, cause: memory allocate failed",
                        VK_ERROR_INITIALIZATION_FAILED);

    /*
     * initialize volk loader to dynamic load about instance function
     * api pointer.
     */
    if ((err = volkInitialize()))
        vrc_error_fatal("Failed to initialize volk loader", err);

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

    const char **required = glfwGetRequiredInstanceExtensions(&count);
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

    if ((err = glfwCreateWindowSurface(driver->instance, window, VK_NULL_HANDLE, &driver->surface)))
        vrc_error_fatal("Failed to create surface", err);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(driver->gpu, &properties);
    printf("Use GPU %s\n", properties.deviceName);

    vrc_printf_device_limits(driver->gpu);

    float priorities = 1.0f;

    vrc_find_queue_index(driver->gpu, driver->surface, &driver->queue_index);

    VkDeviceQueueCreateInfo queue_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = driver->queue_index,
        .queueCount = 1,
        .pQueuePriorities = &priorities,
    };

    std::vector<const char *> device_extensions = {
        "VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering",
        "VK_EXT_dynamic_rendering_unused_attachments"
    };

    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT unusedAttachmentsFeature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT,
        .pNext = nullptr,
        .dynamicRenderingUnusedAttachments = VK_TRUE
    };

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &unusedAttachmentsFeature,
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
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1024 }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1024 * std::size(descriptorPoolSizes),
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
    vkDestroySurfaceKHR(driver->instance, driver->surface, VK_NULL_HANDLE);
    vkDestroyInstance(driver->instance, VK_NULL_HANDLE);
    memdel(driver);
}

void vrc_imgui_init(const VrcDriver *driver, GLFWwindow *window, const VrcSwapchainEXT swapchain)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();

    ImNodes::CreateContext();

    // 设置 ImGui 渲染字体
    io.Fonts->AddFontFromFileTTF("misc/fonts/Microsoft Yahei UI/Microsoft Yahei UI.ttf", 18.0f,
                                 nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    io.FontDefault = io.Fonts->Fonts.back();

    // 设置 ImGui 布局配置文件路径
    io.IniFilename = "../imgui.ini";
    
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = driver->instance;
    init_info.PhysicalDevice = driver->gpu;
    init_info.Device = driver->device;
    init_info.QueueFamily = driver->queue_index;
    init_info.Queue = driver->queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = driver->descriptor_pool;
    init_info.RenderPass = nullptr;
    init_info.MinImageCount = swapchain->min_image_count;
    init_info.ImageCount = swapchain->min_image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain->format,
    };

    ImGui_ImplVulkan_Init(&init_info);
}

void vrc_imgui_terminate()
{
    ImNodes::DestroyContext();
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplVulkan_Shutdown();
}

void vrc_imgui_begin_rendering(VkCommandBuffer U_ASSERT_ONLY command_buffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0xFFFF, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void vrc_imgui_end_rendering(VkCommandBuffer command_buffer)
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::Render();
    ImGui::EndFrame();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void vrc_imgui_begin_viewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("视口");
}

void vrc_imgui_end_viewport()
{
    ImGui::End();
    ImGui::PopStyleVar();
}

void vrc_imgui_begin_node_editor()
{
    ImGui::Begin("节点编辑器");
    ImNodes::BeginNodeEditor();
}

void vrc_imgui_end_node_editor()
{
    ImNodes::EndNodeEditor();
    ImGui::End();
}

VkResult vrc_swapchain_create(const VrcDriver *driver, VrcSwapchainEXT *p_swapchain, VrcSwapchainEXT exist = VK_NULL_HANDLE)
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
    tmp->aspect = (float) tmp->width / tmp->height;

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

    /* copy spir-v shader and other files */
    system("cd ../scripts && shaderc");
    system("cd ../scripts && astcopy");

    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow *window = VK_NULL_HANDLE;
    VrcDriver *driver = VK_NULL_HANDLE;
    VrcSwapchainEXT swapchain = VK_NULL_HANDLE;

    VrcBuffer vertex_buffer = VK_NULL_HANDLE;
    VrcBuffer index_buffer = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ring = VK_NULL_HANDLE;
    VrcPipeline pipeline = VK_NULL_HANDLE;

    window = glfwCreateWindow(1920, 1080, "VeronicaEngine", nullptr, nullptr);
    driver = vrc_driver_init(window);
    vrc_swapchain_create(driver, &swapchain);
    vrc_imgui_init(driver, window, swapchain);

    // 创建 cube vertex buffer 以及 index buffer
    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;
    vrc_load_obj_model("assets/cube/cube.obj", vertices, indices);
    
    VkDeviceSize vertex_buffer_size = std::size(vertices) * sizeof(vertex_t);
    if ((err = vrc_buffer_create(driver, vertex_buffer_size, &vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)))
        vrc_error_fatal("Failed to create vertex buffer", err);
    vrc_memory_write(driver, vertex_buffer, vertex_buffer_size, (void *) std::data(vertices));

    VkDeviceSize index_buffer_size = std::size(indices) * sizeof(uint32_t);
    if ((err = vrc_buffer_create(driver, index_buffer_size, &index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)))
        vrc_error_fatal("Failed to create index buffer", err);
    vrc_memory_write(driver, index_buffer, index_buffer_size, (void *) std::data(indices));
    
    // 加载 cube 贴图
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load("assets/cube/cube_texture.jpg", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    if (!pixels)
        vrc_error_fatal("Failed to load texture image for cube", VK_ERROR_INITIALIZATION_FAILED);

    VrcTexture2D cube_texture;
    if ((err = vrc_texture2d_create(driver, tex_width, tex_height, &cube_texture)) != VK_SUCCESS)
        vrc_error_fatal("Failed to create texture2d for cube texture", err);

    vrc_memory_write(driver, cube_texture, tex_width * tex_height * 4, pixels);
    
    stbi_image_free(pixels);
    
    // 创建渲染管线
    err = vrc_pipeline_create(driver, VK_FORMAT_R8G8B8A8_SRGB, &pipeline);

    if (err)
        vrc_error_fatal("Failed to create graphics pipeline", err);

    glm::mat4 model(1.0f);
    glm::mat4 view(1.0f);
    glm::mat4 proj(1.0f);

    glm::vec3 translation(0.0f);
    glm::vec3 rotation(-59.488f, -0.507, -67.967f);
    glm::vec3 scaling(0.5f);

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;

    // offscreen rendering
    VkDescriptorSet texture_id = VK_NULL_HANDLE;
    VrcTexture2D texture = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_rendering = VK_NULL_HANDLE;
    VkExtent2D viewport_window_size = { 32, 32 };

    bool is_first = true;

    vrc_fence_create(driver, &fence);
    vrc_command_buffer_alloc(driver, &command_buffer_rendering);

    while (!glfwWindowShouldClose(window)) {
        vrc_swapchain_resize_check(driver, &swapchain);

        swapchain->frame = (swapchain->frame + 1) % swapchain->min_image_count;
        vrc_acquire_next_index(driver, swapchain, &swapchain->acquire_index);

        if (!texture || viewport_window_size.width != texture->width || viewport_window_size.height != texture->height) {
            if (texture != VK_NULL_HANDLE)
                vrc_texture2d_destroy(driver, texture);
            vrc_texture2d_create(driver, viewport_window_size.width, viewport_window_size.height, &texture);

            VkCommandBuffer command_buffer_barrier;
            vrc_command_buffer_alloc(driver, &command_buffer_barrier);
            vrc_cmd_begin_once(command_buffer_barrier);
            vrc_memory_image_barrier(command_buffer_barrier,
                                     texture->vk_image,
                                     VK_ACCESS_NONE,
                                     VK_ACCESS_SHADER_READ_BIT,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            vrc_cmd_end_once(command_buffer_barrier);

            vrc_queue_submit(driver,
                             command_buffer_barrier,
                             0,
                             VK_NULL_HANDLE,
                             VK_NULL_HANDLE,
                             0,
                             VK_NULL_HANDLE,
                             fence);

            vkWaitForFences(driver->device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkResetFences(driver->device, 1, &fence);
            vrc_command_buffer_free(driver, command_buffer_barrier);

            texture->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            texture_id = ImGui_ImplVulkan_AddTexture(texture->sampler, texture->vk_view2d, texture->layout);
        }

        PushConstValue push_const;

        view = glm::lookAt(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0, 0.0f));

        proj = glm::perspective(glm::radians(45.0f), (float) viewport_window_size.width / viewport_window_size.height, 0.1f, 100.0f);
        proj[1][1] *= -1;

        push_const.mvp = proj * view * model;

        // 离屏渲染
        vrc_cmd_begin_rendering(command_buffer_rendering, texture->width, texture->height, texture->vk_view2d);

        vrc_cmd_bind_pipeline(command_buffer_rendering, pipeline);
        vrc_cmd_bind_descriptor_set(command_buffer_rendering, pipeline, pipeline->vk_sampler2d_descriptor);
        vrc_descriptor_set_write(driver, pipeline->vk_sampler2d_descriptor, cube_texture);
        vrc_cmd_push_constants(command_buffer_rendering, pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstValue), &push_const);
        vrc_cmd_bind_vertex_buffer(command_buffer_rendering, vertex_buffer);
        vrc_cmd_bind_index_buffer(command_buffer_rendering, index_buffer);
        // vrc_cmd_draw(command_buffer_rendering, std::size(vertices));
        vrc_cmd_draw_indexed(command_buffer_rendering, std::size(indices));

        vrc_cmd_end_rendering(command_buffer_rendering);

        vrc_queue_submit(driver,
                         command_buffer_rendering,
                         0,
                         VK_NULL_HANDLE,
                         VK_NULL_HANDLE,
                         0,
                         VK_NULL_HANDLE,
                         fence);

        vkWaitForFences(driver->device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(driver->device, 1, &fence);
        vkResetCommandBuffer(command_buffer_rendering, 0);

        command_buffer_ring = swapchain->command_buffers[swapchain->frame];
        VkImageView view2d = swapchain->resources[swapchain->acquire_index].image_view;
        vrc_cmd_begin_rendering(command_buffer_ring, swapchain->width, swapchain->height, view2d);
        vrc_imgui_begin_rendering(command_buffer_ring);

        // 控制 MVP 矩阵滑动组件
        ImGui::Begin("MVP");
        bool changed = false;
        changed |= ImGui::DragFloat3("旋转", glm::value_ptr(rotation), 0.05f);
        changed |= ImGui::DragFloat3("平移", glm::value_ptr(translation), 0.05f);
        changed |= ImGui::DragFloat3("缩放", glm::value_ptr(scaling), 0.05f);
        ImGui::End();

        if (changed || is_first) {
            // T
            model = glm::translate(glm::mat4(1.0f), translation);

            // R
            static float speed = 8.0f;
            model = glm::rotate(model, glm::radians(rotation.x * speed), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotation.y * speed), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotation.z * speed), glm::vec3(0.0f, 0.0f, 1.0f));

            // S
            model = glm::scale(model, scaling);

            is_first = false;
        }

        // 渲染 Viewport 展示离屏渲染的图像内容
        vrc_imgui_begin_viewport();
        ImVec2 wsize = ImGui::GetWindowSize();
        ImVec2 wpos = ImGui::GetWindowPos();
        viewport_window_size.width = wsize.x;
        viewport_window_size.height = wsize.y;
        ImGui::Image((ImTextureID) texture_id, { (float) texture->width, (float) texture->height });

        // ImGuizmo
        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(wpos.x, wpos.y, wsize.x, wsize.y);

        if (ImGui::IsKeyPressed(ImGuiKey_T))
            operation = ImGuizmo::TRANSLATE;

        if (ImGui::IsKeyPressed(ImGuiKey_R))
            operation = ImGuizmo::ROTATE;

        if (ImGui::IsKeyPressed(ImGuiKey_S))
            operation = ImGuizmo::SCALE;

        //
        // 前面已经反转了 Y 轴，为什么这里还要再次反转 Y 轴？
        //
        // 因为我们目前的投影矩阵是使用 GLM 的 perspective 函数生成投影矩阵（projection），
        // 而 GLM 本质是为 OpenGL 准备的函数库，所以在 OpenGL 的右手坐标系中 Y 轴是向上的，
        // 在 Vulkan 中使用的是左手坐标系，Y轴向下，所以需要反转一次 Y 轴。
        //
        // 而此处又反转了一次 Y 轴是因为 ImGuizmo 使用的 OpenGL 标准，它的投影是 Y 轴向上的，
        // 所以需要再次反转 Y 轴以匹配 ImGuizmo 的坐标系。
        //
        proj[1][1] *= -1;
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                             operation, ImGuizmo::LOCAL,
                             glm::value_ptr(model), nullptr, nullptr);

        if (ImGuizmo::IsUsing())
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scaling));

        //
        // 此处再次反转 Y 轴是回了 projection 矩阵参与后续的着色器计算，让投影矩阵的坐标系
        // 回归到 Vulkan 的坐标系中。
        //
        proj[1][1] *= -1;

        vrc_imgui_end_viewport();

        // imnodes
        vrc_imgui_begin_node_editor();

        for (int k = 0; k < 3; k++) {
            {
                int base_id = k * 1024;
                ImNodes::BeginNode(base_id);

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("输出节点");
                ImNodes::EndNodeTitleBar();

                static const char *input_name[3] = {
                    "R", "G", "B"
                };

                ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(0, 255, 0, 255));
                for (int i = 0; i < 3; i++) {
                    ImNodes::BeginInputAttribute(base_id + 123 * (i + 1));
                    ImGui::Text(input_name[i]);
                    ImNodes::EndInputAttribute();
                }
                ImNodes::PopColorStyle();

                const int output_attr_id = base_id + 231;
                ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(255, 0, 0, 255));
                ImNodes::BeginOutputAttribute(output_attr_id);
                ImGui::Indent(60);
                ImGui::Text("输出颜色");
                ImNodes::EndOutputAttribute();
                ImNodes::PopColorStyle();

                ImNodes::EndNode();
            }
        }

        for (const auto& link : editor_links) {
            ImNodes::Link(link.id, link.start_attr, link.end_attr);
        }

        vrc_imgui_end_node_editor();

        int start_attr, end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
            static int link_id = 0;
            editor_links.emplace_back((++link_id), start_attr, end_attr);
        }

        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id))
        {
            auto iter = std::find_if(
                editor_links.begin(), editor_links.end(), [link_id](const Link& link) -> bool {
                    return link.id == link_id;
                });
            editor_links.erase(iter);
        }

        vrc_imgui_end_rendering(command_buffer_ring);
        vrc_cmd_end_rendering(command_buffer_ring);

        vrc_present_submit(driver, command_buffer_ring, swapchain);

        vkWaitForFences(driver->device, 1, &(swapchain->fence[swapchain->frame]), VK_TRUE, UINT64_MAX);
        vkResetFences(driver->device, 1, &(swapchain->fence[swapchain->frame]));
        vkResetCommandBuffer(command_buffer_ring, 0);

        glfwPollEvents();
    }

    // 离屏渲染对象销毁
    ImGui_ImplVulkan_RemoveTexture(texture_id);
    vrc_texture2d_destroy(driver, texture);
    vrc_texture2d_destroy(driver, cube_texture);
    vrc_fence_destroy(driver, fence);
    vrc_pipeline_destroy(driver, pipeline);
    vrc_buffer_destroy(driver, vertex_buffer);
    vrc_buffer_destroy(driver, index_buffer);

    vrc_imgui_terminate();
    vrc_swapchain_destroy(driver, swapchain);

    vrc_driver_destroy(driver);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
