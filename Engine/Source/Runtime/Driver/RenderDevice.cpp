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

#ifdef USE_VOLK_LOADER
#  define VOLK_IMPLEMENTATION
#endif /* USE_VOLK_LOADER */

#define VMA_IMPLEMENTATION

#include "RenderDevice.h"

#ifdef _WIN32
#  include <windows.h>
#  include <vulkan/vulkan_win32.h>
#endif /* _WIN32 */

#include <Logger.h>
#include <Error.h>
#include <Vector.h>

#include "VulkanUtils.h"

RenderDevice::RenderDevice(Window* pWindow) : window(pWindow)
{
    VkResult err;

#ifdef USE_VOLK_LOADER
    err = volkInitialize();
    GOGH_ASSERT(!err && "volkInitialize()");
    GOGH_LOGGER_DEBUG("[Vulkan] Volk initialize successful");
#endif /* USE_VOLK_LOADER */
    
    /* 初始化 */
    _InitVkInstance();
    _InitVkSurfaceKHR();
    _InitVKDevice();
    _InitVMAAllocator();
    _InitVkCommandPool();
    _InitVkDescriptorPool();
}

RenderDevice::~RenderDevice()
{
    vkDestroyDescriptorPool(device, descriptorPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(device, commandPool, VK_NULL_HANDLE);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, VK_NULL_HANDLE);
    vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
    vkDestroyInstance(instance, VK_NULL_HANDLE);
}

void RenderDevice::_InitVkInstance()
{
    VkResult err;
    
    vkEnumerateInstanceVersion(&apiVersion);

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vernak Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Vernak Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = this->apiVersion
    };

    Vector<const char *> extensions = {
        "VK_KHR_surface",
#ifdef _WIN32
        "VK_KHR_win32_surface",
#endif /* _WIN32 */
    };

    Vector<const char *> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = (uint32_t) std::size(layers),
        .ppEnabledLayerNames = std::data(layers),
        .enabledExtensionCount = (uint32_t) std::size(extensions),
        .ppEnabledExtensionNames = std::data(extensions),
    };

    err = vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);
    VK_ERROR_CHECK(err, "vkCreateInstance(...)");

    GOGH_LOGGER_DEBUG("[Vulkan] Create instance successful");

#ifdef USE_VOLK_LOADER
    volkLoadInstance(instance);
    GOGH_LOGGER_DEBUG("[Vulkan] Volk load instance proc addr successful");
#endif
}

void RenderDevice::_InitVkSurfaceKHR()
{
    VkResult err;
    
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = (HWND) window->GetNativeWindow(),
    };

    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    err = vkCreateWin32SurfaceKHR(instance, &win32SurfaceCreateInfo, VK_NULL_HANDLE, &surface);
    GOGH_ASSERT(!err && "vkCreateWin32SurfaceKHR(...)");

    GOGH_LOGGER_DEBUG("[Vulkan] Create win32 surface khr successful");
}

void RenderDevice::_InitVKDevice()
{
    VkResult err;

    uint32_t count;
    err = vkEnumeratePhysicalDevices(instance, &count, VK_NULL_HANDLE);
    VK_ERROR_CHECK(err, "vkEnumeratePhysicalDevices(...)");
    
    Vector<VkPhysicalDevice> devices(count);
    err = vkEnumeratePhysicalDevices(instance, &count, std::data(devices));
    VK_ERROR_CHECK(err, "vkEnumeratePhysicalDevices(...)");

    physicalDevice = VulkanUtils::PickDiscreteDevice(devices);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    GOGH_LOGGER_INFO("[Vulkan] Use GPU %s", properties.deviceName);

    float priorities = 1.0f;

    VulkanUtils::FindQueueIndex(physicalDevice, surface, &queueIndex);

    VkDeviceQueueCreateInfo queue_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &priorities,
    };

    Vector<const char *> extensions = {
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
        .enabledExtensionCount = (uint32_t) std::size(extensions),
        .ppEnabledExtensionNames = std::data(extensions),
    };

    err = vkCreateDevice(physicalDevice, &device_ci, VK_NULL_HANDLE, &device);
    VK_ERROR_CHECK(err, "Failed to create logic device");

    GOGH_LOGGER_DEBUG("[Vulkan] Create device successful");
    
#ifdef USE_VOLK_LOADER
    volkLoadDevice(device);
    GOGH_LOGGER_DEBUG("[Vulkan] Volk load device proc addr successful");
#endif
}

void RenderDevice::_InitVMAAllocator()
{
    VkResult err;

    VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };
    
    VmaAllocatorCreateInfo allocatorCreateInfo = {
        .physicalDevice = physicalDevice,
        .device = device,
        .pVulkanFunctions = &functions,
        .instance = instance,
    };

    err = vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    VK_ERROR_CHECK(err, "Failed to create vma allocator");

    GOGH_LOGGER_DEBUG("[Vulkan] VMA allocator create successful");
}

void RenderDevice::_InitVkCommandPool()
{
    VkResult err;
    
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueIndex,
    };

    err = vkCreateCommandPool(device, &commandPoolCreateInfo, VK_NULL_HANDLE, &commandPool);
    VK_ERROR_CHECK(err, "Failed to create command pool");

    GOGH_LOGGER_DEBUG("[Vulkan] Create command pool successful");
}

void RenderDevice::_InitVkDescriptorPool()
{
    VkResult err;
    
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
    VK_ERROR_CHECK(err, "Failed to create descriptor pool");

    GOGH_LOGGER_DEBUG("[Vulkan] Create descriptor pool successful");
}