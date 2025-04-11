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
#include "mctr.h"

#include <stdio.h>

#include <vronk/typedef.h>

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

#include "utils/ioutils.h"

// std
#include <vector>
#include <algorithm>

void error_fatal(const char *msg, VkResult err)
{
        fprintf(stderr, "Error: %s (Result=%d)\n", msg, err);
        exit(EXIT_FAILURE);
}

struct VrakDriver {
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
        VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
} *VrakPipeline;

typedef struct VrakTexture2D_T {
        VkImage vk_image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocation_info = {};
        VkImageView vk_view2d = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t width = 0;
        uint32_t height = 0;
} *VrakTexture2D;

struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
};

static Vertex vertices[] = {
        {{0.0f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
};

void swapchain_destroy(const VrakDriver *driver, VrakSwapchainEXT swapchain);

void pipeline_destroy(const VrakDriver *driver, VrakPipeline pipeline);

void texture2d_destroy(const VrakDriver *driver, VrakTexture2D texture);

VkPhysicalDevice pick_discrete_device(const std::vector<VkPhysicalDevice> &devices)
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
#endif /* USE_GLFW */

void find_queue_index(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *p_index)
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

VkResult fence_create(const VrakDriver *driver, VkFence *p_fence)
{
        VkResult err;

        VkFenceCreateInfo fenceCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };

        return vkCreateFence(driver->device, &fenceCreateInfo, VK_NULL_HANDLE, p_fence);
}

void fence_destroy(const VrakDriver *driver, VkFence fence)
{
        vkDestroyFence(driver->device, fence, VK_NULL_HANDLE);
}

VkResult semaphore_create(const VrakDriver *driver, VkSemaphore *p_semaphore)
{
        VkResult err;

        VkSemaphoreCreateInfo semaphoreCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        return vkCreateSemaphore(driver->device, &semaphoreCreateInfo, VK_NULL_HANDLE, p_semaphore);
}

void semaphore_destroy(const VrakDriver *driver, VkSemaphore semaphore)
{
        vkDestroySemaphore(driver->device, semaphore, VK_NULL_HANDLE);
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

void command_buffer_free(const VrakDriver *driver, VkCommandBuffer command_buffer)
{
        vkFreeCommandBuffers(driver->device, driver->command_pool, 1, &command_buffer);
}

VkResult buffer_create(const VrakDriver *driver, VkDeviceSize size, VrakBuffer *p_buffer)
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

        return vmaCreateBuffer(driver->allocator, &buffer_ci, &allocation_info, &tmp->vk_buffer, &tmp->allocation,
                               &tmp->allocation_info);
}

void buffer_destroy(VrakDriver *driver, VrakBuffer buffer)
{
        vmaDestroyBuffer(driver->allocator, buffer->vk_buffer, buffer->allocation);
        memdel(buffer);
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

VkResult texture2d_create(const VrakDriver *driver, uint32_t w, uint32_t h, VrakTexture2D *p_texture)
{
        VkResult err;
        VrakTexture2D tmp = VK_NULL_HANDLE;

        tmp = memnew<VrakTexture2D_T>();

        tmp->width = w;
        tmp->height = h;
        tmp->format = VK_FORMAT_R8G8B8A8_SRGB;

        auto texture2d_cleanup = [&](VkResult err) {
                if (err) {
                        texture2d_destroy(driver, tmp);
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
                .samples = VK_SAMPLE_COUNT_4_BIT,
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

        if ((err = image_view2d_create(driver, tmp->vk_image, tmp->format, &tmp->vk_view2d)))
                return texture2d_cleanup(err);

        *p_texture = tmp;

        return err;
}

void texture2d_destroy(const VrakDriver *driver, VrakTexture2D texture)
{
        if (!texture)
                return;

        if (texture->vk_view2d != VK_NULL_HANDLE)
                vkDestroyImageView(driver->device, texture->vk_view2d, VK_NULL_HANDLE);

        if (texture->vk_image != VK_NULL_HANDLE)
                vmaDestroyImage(driver->allocator, texture->vk_image, texture->allocation);

        memdel(texture);
}

VkResult pipeline_create(const VrakDriver *driver, VkFormat color, VrakPipeline *p_pipeline)
{
        VkResult err;
        VrakPipeline tmp = VK_NULL_HANDLE;
        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        VkShaderModule fragment_shader_module = VK_NULL_HANDLE;

        tmp = memnew<VrakPipeline_T>();

        /* use cleanup to clear resource when some error. */
        auto pipeline_cleanup = [&](VkResult err) {
                if (vertex_shader_module != VK_NULL_HANDLE)
                        vkDestroyShaderModule(driver->device, vertex_shader_module, VK_NULL_HANDLE);

                if (fragment_shader_module != VK_NULL_HANDLE)
                        vkDestroyShaderModule(driver->device, fragment_shader_module, VK_NULL_HANDLE);

                if (err != VK_SUCCESS)
                        pipeline_destroy(driver, tmp);

                return err;
        };

        if ((err = load_shader_module(driver->device, "shaders/spir-v/simple_vertex.spv", &vertex_shader_module)))
                return pipeline_cleanup(err);

        if ((err = load_shader_module(driver->device, "shaders/spir-v/simple_fragment.spv", &fragment_shader_module)))
                return pipeline_cleanup(err);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        };

        if ((err = vkCreatePipelineLayout(driver->device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE,
                                          &tmp->vk_pipeline_layout)))
                return pipeline_cleanup(err);

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
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(Vertex, position)
                },
                {
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(Vertex, position)
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
                .rasterizationSamples = VK_SAMPLE_COUNT_4_BIT,
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
                .pAttachments = &pipelineColorBlendAttachmentState
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
                return pipeline_cleanup(err);

        *p_pipeline = tmp;

        return pipeline_cleanup(err);
}

void pipeline_destroy(const VrakDriver *driver, VrakPipeline pipeline)
{
        if (!pipeline)
                return;

        if (pipeline->vk_pipeline_layout != VK_NULL_HANDLE)
                vkDestroyPipelineLayout(driver->device, pipeline->vk_pipeline_layout, VK_NULL_HANDLE);

        if (pipeline->vk_pipeline != VK_NULL_HANDLE)
                vkDestroyPipeline(driver->device, pipeline->vk_pipeline, VK_NULL_HANDLE);

        memdel(pipeline);
}

void memory_read(const VrakDriver *driver, VrakBuffer buffer, size_t size, void *dst)
{
        void *src;
        vmaMapMemory(driver->allocator, buffer->allocation, &src);
        memcpy(dst, src, size);
        vmaUnmapMemory(driver->allocator, buffer->allocation);
}

void memory_write(const VrakDriver *driver, VrakBuffer buffer, size_t size, void *src)
{
        void *dst;
        vmaMapMemory(driver->allocator, buffer->allocation, &dst);
        memcpy(dst, src, size);
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
                &barrier);
}

void cmd_begin(VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                         VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        };

        vkBeginCommandBuffer(command_buffer, &begin_info);
}

void cmd_end(VkCommandBuffer command_buffer)
{
        vkEndCommandBuffer(command_buffer);
}

void cmd_begin_rendering(VkCommandBuffer command_buffer, VrakTexture2D target)
{
        cmd_begin(command_buffer);

        VkRenderingAttachmentInfo renderingAttachmentInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = target->vk_view2d,
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
                        .extent = {target->width, target->height}},
                .layerCount = 1,
                .pColorAttachments = &renderingAttachmentInfo,
                .pDepthAttachment = VK_NULL_HANDLE,
        };

        vkCmdBeginRendering(command_buffer, &renderingInfo);

        VkViewport viewport = {
                .width = static_cast<float>(target->width),
                .height = static_cast<float>(target->height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };

        VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {target->width, target->height}
        };

        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void cmd_end_rendering(VkCommandBuffer command_buffer)
{
        vkCmdEndRendering(command_buffer);
        cmd_end(command_buffer);
}

void cmd_bind_pipeline(VkCommandBuffer command_buffer, VrakPipeline pipeline)
{
        vkCmdBindPipeline(command_buffer, pipeline->bindpoint, pipeline->vk_pipeline);
}

void cmd_bind_vertex_buffer(VkCommandBuffer command_buffer, VrakBuffer vertex_buffer)
{
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer->vk_buffer, &offsets);
}

void cmd_draw(VkCommandBuffer command_buffer)
{
        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

VkResult queue_submit(const VrakDriver *driver, VkCommandBuffer command_buffer)
{
        VkSubmitInfo submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &command_buffer,
        };

        return vkQueueSubmit(driver->queue, 1, &submitInfo, VK_NULL_HANDLE);
}

VkResult queue_wait_idle(const VrakDriver *driver)
{
        return vkQueueWaitIdle(driver->queue);
}

#ifdef USE_GLFW
VrakDriver *vrak_driver_initialize(GLFWwindow *window)
#else

VrakDriver *vrak_driver_initialize()
#endif /* USE_GLFW */
{
        VkResult err;

        VrakDriver *driver = memnew<VrakDriver>();

#ifdef USE_VOLK
        /*
         * initialize volk loader to dynamic load about instance function
         * api pointer.
         */
        if ((err = volkInitialize()))
                error_fatal("Failed to initialize volk loader", err);
#endif /* USE_VOLK */

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

#ifdef USE_GLFW
        const char **required = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char *> extensions;

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
                error_fatal("Failed to create instance", err);

#ifdef USE_VOLK
        volkLoadInstance(driver->instance);
#endif /* USE_VOLK */

        if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, VK_NULL_HANDLE)))
                error_fatal("Failed to enumerate physical device list count", err);

        printf("Enumerate vulkan physical device count: %u\n", count);

        std::vector<VkPhysicalDevice> devices(count);
        if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, std::data(devices))))
                error_fatal("Failed to enumerate physical device list data", err);

        driver->gpu = pick_discrete_device(devices);

#ifdef USE_GLFW
        if ((err = glfwCreateWindowSurface(driver->instance, window, VK_NULL_HANDLE, &driver->surface)))
                error_fatal("Failed to create surface", err);
#endif /* USE_GLFW */

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(driver->gpu, &properties);
        printf("Use GPU %s\n", properties.deviceName);

        float priorities = 1.0f;

#ifdef USE_GLFW
        find_queue_index(driver->gpu, driver->surface, &driver->queue_index);
#else
        find_queue_index(driver->gpu, VK_NULL_HANDLE, &driver->queue_index);
#endif /* USE_GLFW */

        VkDeviceQueueCreateInfo queue_ci = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = driver->queue_index,
                .queueCount = 1,
                .pQueuePriorities = &priorities,
        };

        std::vector<const char *> device_extensions;

#ifdef USE_GLFW
        extensions.push_back("VK_KHR_swapchain");
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
                error_fatal("Failed to create logic device", err);

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

void vrak_driver_destroy(VrakDriver *driver)
{
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
VkResult swapchain_create(VrakDriver *driver, VrakSwapchainEXT *p_swapchain)
{
        VkResult err;
        VrakSwapchainEXT tmp = VK_NULL_HANDLE;
        std::vector<VkImage> images;

        tmp = memnew<VrakSwapchainEXT_T>();
        
        /* use cleanup to clear resource when some error. */
        auto swapchain_cleanup = [&](VkResult err) {
                swapchain_destroy(driver, tmp);
                return err;
        };

        if ((err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(driver->gpu, driver->surface, &tmp->capabilities)))
                return swapchain_cleanup(err);

        uint32_t min = tmp->capabilities.minImageCount;
        uint32_t max = tmp->capabilities.maxImageCount;
        tmp->min_image_count = std::clamp(min + 1, min, max);

        *p_swapchain = tmp;

        VkSurfaceFormatKHR surface_format;
        if ((err = pick_suitable_surface_format(driver, &surface_format)))
                return swapchain_cleanup(err);

        tmp->format = surface_format.format;
        tmp->color_space = surface_format.colorSpace;

        VkSwapchainCreateInfoKHR swapchain_ci = {
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
                return swapchain_cleanup(err);

        uint32_t count;
        if ((err = vkGetSwapchainImagesKHR(driver->device, tmp->vk_swapchain, &count, VK_NULL_HANDLE)))
                return swapchain_cleanup(err);

        images.resize(count);
        if ((err = vkGetSwapchainImagesKHR(driver->device, tmp->vk_swapchain, &count, std::data(images))))
                return swapchain_cleanup(err);

        tmp->resources.resize(tmp->min_image_count);
        for (int i = 0; i < tmp->min_image_count; ++i) {
                tmp->resources[i].image = images[i];
                if ((err = image_view2d_create(driver, tmp->resources[i].image, tmp->format, &(tmp->resources[i].image_view))))
                        return swapchain_cleanup(err);
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
#endif /* USE_GLFW */

int main()
{
        VkResult err;

        /*
         * close stdout and stderr write to buf, let direct
         * output.
         */
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

#ifdef USE_GLFW
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWwindow* window = VK_NULL_HANDLE;
#endif /* USE_GLFW */

        VrakDriver *driver = VK_NULL_HANDLE;

#ifdef USE_GLFW
        VrakSwapchainEXT swapchain = VK_NULL_HANDLE;
#endif /* USE_GLFW */

        VrakBuffer vertex_buffer = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VrakPipeline pipeline = VK_NULL_HANDLE;
        VrakTexture2D texture = VK_NULL_HANDLE;

#ifdef USE_GLFW
        window = glfwCreateWindow(800, 600, "Veronak Cube", nullptr, nullptr);
        driver = vrak_driver_initialize(window);
        swapchain_create(driver, &swapchain);
#else
        driver = vrak_driver_initialize();
#endif /* USE_GLFW */

        // create vertex buffer
        if ((err = buffer_create(driver, sizeof(vertices), &vertex_buffer)))
                error_fatal("Failed to create vertex buffer", err);

        memory_write(driver, vertex_buffer, sizeof(vertices), (void *) vertices);

        command_buffer_alloc(driver, &command_buffer);

#ifdef USE_GLFW
        err = pipeline_create(driver, swapchain->format, &pipeline);
#else
        err = pipeline_create(driver, VK_FORMAT_R8G8B8A8_SRGB, &pipeline);
#endif /* USE_GLFW */

        if (err) error_fatal("Failed to create graphics pipeline", err);

        if ((err = texture2d_create(driver, 800, 600, &texture)))
                error_fatal("Failed to create texture 2d", err);

#ifdef USE_GLFW
        while (!glfwWindowShouldClose(window)) {
#endif /* USE_GLFW */

        cmd_begin_rendering(command_buffer, texture);
        cmd_bind_pipeline(command_buffer, pipeline);
        cmd_bind_vertex_buffer(command_buffer, vertex_buffer);
        cmd_draw(command_buffer);
        cmd_end_rendering(command_buffer);
        queue_submit(driver, command_buffer);
        queue_wait_idle(driver);

#ifdef USE_GLFW
        glfwPollEvents();
}
#endif /* USE_GLFW */

        texture2d_destroy(driver, texture);
        pipeline_destroy(driver, pipeline);
        command_buffer_free(driver, command_buffer);
        buffer_destroy(driver, vertex_buffer);

#ifdef USE_GLFW
        swapchain_destroy(driver, swapchain);
#endif /* USE_GLFW */

        vrak_driver_destroy(driver);

#ifdef USE_GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
#endif /* USE_GLFW */

        return 0;
}
