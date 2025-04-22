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
#include "VulkanRenderDevice.h"

#ifdef WIN32
#include <vulkan/vulkan_win32.h>
#endif

VulkanRenderDevice::VulkanRenderDevice(const Window* _window) : window(_window)
{
    VkResult err;
    uint32_t count;

    driver = MemoryNew<VulkanDriver>();

    /*
     * initialize volk loader to dynamic load about instance function
     * api pointer.
     */
    if ((err = volkInitialize()))
        ERROR_FATAL("Failed to initialize volk loader");

    if ((err = vkEnumerateInstanceVersion(&driver->apiVersion)))
        ERROR_FATAL("Can't not get instance version");

    printf("Vulkan %u.%u.%u\n",
           VK_VERSION_MAJOR(driver->apiVersion),
           VK_VERSION_MINOR(driver->apiVersion),
           VK_VERSION_PATCH(driver->apiVersion));

    VkApplicationInfo info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Veronak Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Veronak Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = driver->apiVersion
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
        ERROR_FATAL("Failed to create instance");

#ifdef USE_VOLK_LOADER
    volkLoadInstance(driver->instance);
#endif /* USE_VOLK_LOADER */

    if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, VK_NULL_HANDLE)))
        ERROR_FATAL("Failed to enumerate physical device list count");

    std::vector<VkPhysicalDevice> devices(count);
    if ((err = vkEnumeratePhysicalDevices(driver->instance, &count, std::data(devices))))
        ERROR_FATAL("Failed to enumerate physical device list data");

    driver->physicalDevice = VulkanUtils::PickDiscreteDevice(devices);

    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = VK_NULL_HANDLE,
        .hwnd = (HWND) window->GetNativeHandle(),
    };

    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(driver->instance, "vkCreateWin32SurfaceKHR");
    vkCreateWin32SurfaceKHR(driver->instance, &win32SurfaceCreateInfo, nullptr, &driver->surface);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(driver->physicalDevice, &properties);
    printf("Use GPU %s\n", properties.deviceName);

    float priorities = 1.0f;

    VulkanUtils::FindQueueIndex(driver->physicalDevice, driver->surface, &driver->queueIndex);

    VkDeviceQueueCreateInfo queue_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = driver->queueIndex,
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

    if ((err = vkCreateDevice(driver->physicalDevice, &device_ci, VK_NULL_HANDLE, &driver->device)))
        ERROR_FATAL("Failed to create logic device");

#ifdef USE_VOLK_LOADER
    volkLoadDevice(driver->device);
#endif /* USE_VOLK_LOADER */

    vkGetDeviceQueue(driver->device, driver->queueIndex, 0, &driver->queue);

    VkCommandPoolCreateInfo command_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = driver->queueIndex,
    };

    if ((err = vkCreateCommandPool(driver->device, &command_pool_ci, VK_NULL_HANDLE, &driver->commandPool)))
        ERROR_FATAL("Failed to create command pool");

    VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo allocator_ci = {
        .physicalDevice = driver->physicalDevice,
        .device = driver->device,
        .pVulkanFunctions = &functions,
        .instance = driver->instance,
    };

    if ((err = vmaCreateAllocator(&allocator_ci, &driver->allocator)))
        ERROR_FATAL("Failed to create VMA allocator");

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

    if ((err = vkCreateDescriptorPool(driver->device, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &driver->descriptorPool)))
        ERROR_FATAL("Failed to create descriptor pool");
}

VulkanRenderDevice::~VulkanRenderDevice()
{
    vkDestroyDescriptorPool(driver->device, driver->descriptorPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(driver->device, driver->commandPool, VK_NULL_HANDLE);
    vmaDestroyAllocator(driver->allocator);
    vkDestroyDevice(driver->device, VK_NULL_HANDLE);
    vkDestroySurfaceKHR(driver->instance, driver->surface, VK_NULL_HANDLE);
    vkDestroyInstance(driver->instance, VK_NULL_HANDLE);

    MemoryDelete(driver);
}

Buffer *VulkanRenderDevice::CreateBuffer(size_t size, BufferUsageFlags usage)
{
    return MemoryNew<VulkanBuffer>(driver, size, usage);
}

void VulkanRenderDevice::DestroyBuffer(Buffer *buffer)
{
    return MemoryDelete(buffer);
}