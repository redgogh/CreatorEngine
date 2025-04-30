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
#include <MM.h>

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

RenderDevice::BufferVk* RenderDevice::CreateBuffer(size_t size, VkBufferUsageFlags usage)
{
    BufferVk* buffer = MemoryNew<BufferVk>();

    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer->vkBuffer, &buffer->allocation, &buffer->allocationInfo);
    
    buffer->size = size;
    
    return buffer;
}

void RenderDevice::DestroyBuffer(RenderDevice::BufferVk* buffer)
{
    MemoryDelete(buffer);
}

void RenderDevice::ReadBuffer(RenderDevice::BufferVk *buffer, size_t offset, size_t size, void *dst)
{
    void* src;
    vmaMapMemory(allocator, buffer->allocation, &src);
    memcpy(dst, (static_cast<char*>(src) + offset), size);
    vmaUnmapMemory(allocator, buffer->allocation);
}

void RenderDevice::WriteBuffer(BufferVk* buffer, size_t offset, size_t size, const void* src)
{
    void* dst;
    vmaMapMemory(allocator, buffer->allocation, &dst);
    memcpy((static_cast<char*>(dst) + offset), src, size);
    vmaUnmapMemory(allocator, buffer->allocation);
}

RenderDevice::SwapchainVkEXT* RenderDevice::CreateSwapchainEXT(SwapchainVkEXT* oldSwapchainEXT)
{
    VkResult err;
    std::vector<VkImage> images;

    GOGH_LOGGER_INFO("[Vulkan] Creating new swapchain (oldSwapchainEXT: %p)", oldSwapchainEXT);
    
    SwapchainVkEXT* swapchain = MemoryNew<SwapchainVkEXT>();

    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchain->capabilities);
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_ERROR("[Vulkan] Failed to get surface capabilities: %d", err);
        return VK_NULL_HANDLE;
    }
    
    GOGH_LOGGER_DEBUG("[Vulkan] Surface capabilities retrieved successfully");
    
    uint32_t min = swapchain->capabilities.minImageCount;
    uint32_t max = swapchain->capabilities.maxImageCount;
    swapchain->minImageCount = std::clamp(min + 1, min, max);

    GOGH_LOGGER_DEBUG("[Vulkan] Swapchain image count: min=%u, max=%u, using=%u", min, max, swapchain->minImageCount);

    swapchain->width = swapchain->capabilities.currentExtent.width;
    swapchain->height = swapchain->capabilities.currentExtent.height;
    swapchain->aspect = (float) swapchain->width / swapchain->height;

    GOGH_LOGGER_INFO("[Vulkan] Swapchain extent: %ux, %u (aspect: %.2f)", swapchain->width, swapchain->height, swapchain->aspect);
    
    VkSurfaceFormatKHR surfaceFormat;
    err = VulkanUtils::PickSurfaceFormat(physicalDevice, surface, &surfaceFormat);
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_ERROR("[Vulkan] Failed to pick suitable surface format");
        return VK_NULL_HANDLE;
    }
    
    GOGH_LOGGER_DEBUG("[Vulkan] Selected surface format: %d, color space: %d", surfaceFormat.format, surfaceFormat.colorSpace);
    
    swapchain->format = surfaceFormat.format;
    swapchain->colorSpace = surfaceFormat.colorSpace;

    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = swapchain->minImageCount,
        .imageFormat = swapchain->format,
        .imageColorSpace = swapchain->colorSpace,
        .imageExtent = swapchain->capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = swapchain->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchainEXT != VK_NULL_HANDLE ? oldSwapchainEXT->vkSwapchainKHR : VK_NULL_HANDLE,
    };
    
    err = vkCreateSwapchainKHR(device, &swapchainCreateInfoKHR, VK_NULL_HANDLE, &swapchain->vkSwapchainKHR);

    if (err != VK_SUCCESS) {
        GOGH_LOGGER_ERROR("[Vulkan] Failed to create swapchain: %d", err);
        return VK_NULL_HANDLE;
    }
    
    GOGH_LOGGER_INFO("[Vulkan] Swapchain created successfully");


    if (oldSwapchainEXT != VK_NULL_HANDLE)
        DestroySwapchainEXT(oldSwapchainEXT);
    
    uint32_t count;
    vkGetSwapchainImagesKHR(device, swapchain->vkSwapchainKHR, &count, VK_NULL_HANDLE);

    images.resize(count);
    vkGetSwapchainImagesKHR(device, swapchain->vkSwapchainKHR, &count, std::data(images));

    swapchain->resources.resize(swapchain->minImageCount);
    swapchain->acquireIndexSemaphore.resize(swapchain->minImageCount);
    swapchain->renderFinishSemaphore.resize(swapchain->minImageCount);
    swapchain->fence.resize(swapchain->minImageCount);
    swapchain->commandBuffers.resize(swapchain->minImageCount);

    GOGH_LOGGER_INFO("[Vulkan] Initializing %u swapchain resources...", swapchain->minImageCount);
    for (int i = 0; i < swapchain->minImageCount; ++i) {
        swapchain->resources[i].image = images[i];

        err = _CreateImageView(swapchain->resources[i].image, swapchain->format, &(swapchain->resources[i].imageView));
        err = _CreateSemaphore(&(swapchain->acquireIndexSemaphore[i]));
        err = _CreateSemaphore(&(swapchain->renderFinishSemaphore[i]));
        err = _CreateFence(&(swapchain->fence[i]));
        err = _CommandBufferAllocate(&(swapchain->commandBuffers[i]));

        GOGH_LOGGER_DEBUG("[Vulkan] Initialized swapchain resource %d/%u", i + 1, swapchain->minImageCount);
    }

    GOGH_LOGGER_INFO("[Vulkan] Swapchain created and initialized successfully");
    
    return swapchain;
}

void RenderDevice::DestroySwapchainEXT(RenderDevice::SwapchainVkEXT *swapchain)
{
    if (swapchain == VK_NULL_HANDLE)
        return;

    vkQueueWaitIdle(queue);

    GOGH_LOGGER_DEBUG("[Vulkan] Destroying SwapchainEXT, (SwapchainEXT=%p)", swapchain);
    
    for (int i = 0; i < swapchain->minImageCount; ++i) {
        auto resource = swapchain->resources[i];

        if (resource.imageView != VK_NULL_HANDLE)
            _DestroyImageView(resource.imageView);

        if (swapchain->acquireIndexSemaphore[i] != VK_NULL_HANDLE)
            _DestroySemaphore(swapchain->acquireIndexSemaphore[i]);

        if (swapchain->renderFinishSemaphore[i] != VK_NULL_HANDLE)
            _DestroySemaphore(swapchain->renderFinishSemaphore[i]);

        if (swapchain->fence[i] != VK_NULL_HANDLE)
            _DestroyFence(swapchain->fence[i]);

        if (swapchain->commandBuffers[i] != VK_NULL_HANDLE)
            _CommandBufferFree(swapchain->commandBuffers[i]);
    }

    vkDestroySwapchainKHR(device, swapchain->vkSwapchainKHR, VK_NULL_HANDLE);
    MemoryDelete(swapchain);
}

VkResult RenderDevice::_CreateImageView(VkImage image, VkFormat format, VkImageView *pImageView)
{
    VkResult err;
    
    VkImageViewCreateInfo imageViewCreateInfo = {
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
    
    err = vkCreateImageView(device, &imageViewCreateInfo, VK_NULL_HANDLE, pImageView);
    
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_WARN("[Vulkan] Failed to creating VkImageView, VkImage=%p, VkFormat=%d", image, format);
        goto TAG_CREATE_IMAGE_VIEW_END;
    }

    GOGH_LOGGER_DEBUG("[Vulkan] Creating VkImageView successful, (VkImage=%p, VkFormat=%d, VkImageView=%p)", image, format, *pImageView);
    
TAG_CREATE_IMAGE_VIEW_END:
    return err;
}

void RenderDevice::_DestroyImageView(VkImageView imageView)
{
    GOGH_LOGGER_DEBUG("[Vulkan] Destroying VkImageView %p", imageView);
    vkDestroyImageView(device, imageView, VK_NULL_HANDLE);
}

VkResult RenderDevice::_CreateSemaphore(VkSemaphore *pSemaphore)
{
    VkResult err;
    
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    err = vkCreateSemaphore(device, &semaphoreCreateInfo, VK_NULL_HANDLE, pSemaphore);
    
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_WARN("[Vulkan] Failed to creating VkSemaphore");
        goto TAG_CREATE_SEMAPHORE_END;
    }

    GOGH_LOGGER_DEBUG("[Vulkan] Creating VkSemaphore successful, (VkSemaphore=%p)", *pSemaphore);

TAG_CREATE_SEMAPHORE_END:
    return err;
}

void RenderDevice::_DestroySemaphore(VkSemaphore semaphore)
{
    GOGH_LOGGER_DEBUG("[Vulkan] Destroying VkSemaphore %p", semaphore);
    vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);
}

VkResult RenderDevice::_CreateFence(VkFence *pFence)
{
    VkResult err;
    
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    err = vkCreateFence(device, &fenceCreateInfo, VK_NULL_HANDLE, pFence);
    
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_WARN("[Vulkan] Failed to creating VkFence");
        goto TAG_CREATE_FENCE_END;
    }

    GOGH_LOGGER_WARN("[Vulkan] Creating VkFence successful, (VkFence=%p)", *pFence);
    
TAG_CREATE_FENCE_END:
    return err;
}

void RenderDevice::_DestroyFence(VkFence fence)
{
    GOGH_LOGGER_DEBUG("[Vulkan] Destroying VkFence %p", fence);
    vkDestroyFence(device, fence, VK_NULL_HANDLE);
}

VkResult RenderDevice::_CommandBufferAllocate(VkCommandBuffer* pCommandBuffer)
{
    VkResult err;
    
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    err = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffer);
    
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_WARN("[Vulkan] Failed allocating VkCommandBuffer %p, (VkCommandPool: %p)", *pCommandBuffer, commandPool);
        return err;
    }

    GOGH_LOGGER_DEBUG("[Vulkan] Allocated VkCommandBuffer: %p (VkCommandPool: %p)", *pCommandBuffer, commandPool);
    return err;
}

void RenderDevice::_CommandBufferFree(VkCommandBuffer commandBuffer)
{
    GOGH_LOGGER_DEBUG("[Vulkan] Free VkCommandBuffer: %p (VkCommandPool: %p)", commandBuffer, commandPool);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void RenderDevice::_InitVkInstance()
{
    VkResult err;
    
    vkEnumerateInstanceVersion(&apiVersion);

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "GOGH_ENG_CORE",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "GOGH_ENG_CORE",
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

    GOGH_LOGGER_DEBUG("[Vulkan] Create instance successful, (instance=%p)", instance);

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

    GOGH_LOGGER_DEBUG("[Vulkan] Create win32 surface khr successful, (surface=%p)", surface);
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

    VulkanUtils::FindQueueIndex(physicalDevice, surface, &queueFamilyIndex);

    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
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

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRenderingFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = (uint32_t) std::size(extensions),
        .ppEnabledExtensionNames = std::data(extensions),
    };

    err = vkCreateDevice(physicalDevice, &deviceCreateInfo, VK_NULL_HANDLE, &device);
    VK_ERROR_CHECK(err, "Failed to create logic device");

    GOGH_LOGGER_DEBUG("[Vulkan] Create device successful, (device=%p)", device);
    
#ifdef USE_VOLK_LOADER
    volkLoadDevice(device);
    GOGH_LOGGER_DEBUG("[Vulkan] Volk load device proc addr successful");
#endif
    
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    GOGH_LOGGER_DEBUG("[Vulkan] Get device queue handle, (queueFamilyIndex=%u, queue=%p)", queueFamilyIndex, queue);
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
        .queueFamilyIndex = queueFamilyIndex,
    };

    err = vkCreateCommandPool(device, &commandPoolCreateInfo, VK_NULL_HANDLE, &commandPool);
    VK_ERROR_CHECK(err, "Failed to create command pool");

    GOGH_LOGGER_DEBUG("[Vulkan] Create command pool successful, (commandPool=%p)", commandPool);
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

    GOGH_LOGGER_DEBUG("[Vulkan] Create descriptor pool successful, (VkDescriptorPool=%p)", descriptorPool);
}