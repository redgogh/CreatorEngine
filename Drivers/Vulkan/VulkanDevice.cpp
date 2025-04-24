/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 RedGogh All rights reserved.                          *|
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

/* Create by Red Gogh on 2025/4/22 */

#include "VulkanDevice.h" 

VulkanDevice::VulkanDevice(const VulkanContext *_ctx) : vkContext(_ctx)
{
    VkResult err;
    uint32_t count;
    
    // ------------ //
    // -- Device -- //
    // ------------ //

    err = vkEnumeratePhysicalDevices(vkContext->GetInstance(), &count, VK_NULL_HANDLE);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to enumerate physical device list count", err);
    
    std::vector<VkPhysicalDevice> devices(count);
    err = vkEnumeratePhysicalDevices(vkContext->GetInstance(), &count, std::data(devices));
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to enumerate physical device list data", err);
    
    physicalDevice = VulkanUtils::PickDiscreteDevice(devices);
    
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    printf("Use GPU %s\n", properties.deviceName);

    float priorities = 1.0f;

    VulkanUtils::FindQueueIndex(physicalDevice, vkContext->GetSurfaceKHR(), &queueIndex);

    VkDeviceQueueCreateInfo queue_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
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

    err = vkCreateDevice(physicalDevice, &device_ci, VK_NULL_HANDLE, &device);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to create logic device", err);

#ifdef USE_VOLK_LOADER
    volkLoadDevice(device);
#endif /* USE_VOLK_LOADER */

    vkGetDeviceQueue(device, queueIndex, 0, &queue);

    // ------------------ //
    // -- Command Pool -- //
    // ------------------ //

    VkCommandPoolCreateInfo command_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueIndex,
    };

    err = vkCreateCommandPool(device, &command_pool_ci, VK_NULL_HANDLE, &commandPool);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to create command pool", err);

    // ------------------- //
    // -- VMA Allocator -- //
    // ------------------- //

    VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo allocator_ci = {
        .physicalDevice = physicalDevice,
        .device = device,
        .pVulkanFunctions = &functions,
        .instance = vkContext->GetInstance(),
    };

    err = vmaCreateAllocator(&allocator_ci, &allocator);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to create VMA allocator", err);

    // --------------------- //
    // -- Descriptor Pool -- //
    // --------------------- //

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

    err = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &descriptorPool);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to create descriptor pool", err);
}

VulkanDevice::~VulkanDevice()
{
    vkDestroyDescriptorPool(device, descriptorPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(device, commandPool, VK_NULL_HANDLE);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, VK_NULL_HANDLE);
}

void VulkanDevice::GetSurfaceCapabilities(VkSurfaceCapabilitiesKHR* pSurfaceCapabilitiesKHR) const
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkContext->GetSurfaceKHR(), pSurfaceCapabilitiesKHR);
}

VkResult VulkanDevice::PickSurfaceFormat(VkSurfaceFormatKHR *pSurfaceFormat) const
{
    VkResult err;

    uint32_t count;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkContext->GetSurfaceKHR(), &count, VK_NULL_HANDLE);
    if (err)
        return err;

    std::vector<VkSurfaceFormatKHR> formats(count);
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkContext->GetSurfaceKHR(), &count, std::data(formats));
    if (err)
        return err;

    for (const auto &item: formats) {
        switch (item.format) {
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_UNORM: {
                *pSurfaceFormat = item;
                goto TAG_PICK_SUITABLE_SURFACE_FORMAT_END;
            }
        }
    }

    assert(count >= 1);
    *pSurfaceFormat = formats[0];

    printf("Can't not found suitable surface format, default use first\n");

TAG_PICK_SUITABLE_SURFACE_FORMAT_END:
    return VK_SUCCESS;
}

VkResult VulkanDevice::CreateFence(VkFence *pFence) const
{
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    return vkCreateFence(device, &fenceCreateInfo, VK_NULL_HANDLE, pFence);
}

void VulkanDevice::DestroyFence(VkFence fence) const
{
    vkDestroyFence(device, fence, VK_NULL_HANDLE);
}

VkResult VulkanDevice::CreateSemaphoreVk(VkSemaphore *pSemaphore) const
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    return vkCreateSemaphore(device, &semaphoreCreateInfo, VK_NULL_HANDLE, pSemaphore);
}

void VulkanDevice::DestroySemaphoreVk(VkSemaphore semaphore) const
{
    vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);
}

VkResult VulkanDevice::CreateImageView(VkImage image, VkFormat format, VkImageView *pImageView) const
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

    return vkCreateImageView(device, &image_view2d_ci, VK_NULL_HANDLE, pImageView);
}

void VulkanDevice::DestroyImageView(VkImageView imageView) const
{
    vkDestroyImageView(device, imageView, VK_NULL_HANDLE);
}

VkResult VulkanDevice::AllocateCommandBuffer(VkCommandBuffer *pCommandBuffer) const
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    return vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffer);
}

void VulkanDevice::FreeCommandBuffer(VkCommandBuffer commandBuffer) const
{
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkResult VulkanDevice::AllocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet *pDescriptorSet) const
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    
    return vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, pDescriptorSet);
}

void VulkanDevice::FreeDescriptorSet(VkDescriptorSet descriptorSet) const
{
    vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
}