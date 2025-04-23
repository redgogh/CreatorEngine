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
    VK_CHECK_FATAL_ERROR("Failed to enumerate physical device list count", err);
    
    std::vector<VkPhysicalDevice> devices(count);
    err = vkEnumeratePhysicalDevices(vkContext->GetInstance(), &count, std::data(devices));
    VK_CHECK_FATAL_ERROR("Failed to enumerate physical device list data", err);
    
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
    VK_CHECK_FATAL_ERROR("Failed to create logic device", err);

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
    VK_CHECK_FATAL_ERROR("Failed to create command pool", err);

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
    VK_CHECK_FATAL_ERROR("Failed to create VMA allocator", err);

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
    VK_CHECK_FATAL_ERROR("Failed to create descriptor pool", err);
}

VulkanDevice::~VulkanDevice()
{
    vkDestroyDescriptorPool(device, descriptorPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(device, commandPool, VK_NULL_HANDLE);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, VK_NULL_HANDLE);
}