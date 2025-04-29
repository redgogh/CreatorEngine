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

#pragma once

#ifdef USE_VOLK_LOADER
#  include <volk/volk.h>
#else
#  include <vulkan/vulkan.h>
#endif /* USE_VOLK_LOADER */

#include <vma/vk_mem_alloc.h>

#include "Window/Window.h"

class RenderDevice
{
public:
    RenderDevice(Window* pWindow);
   ~RenderDevice();

    struct BufferInfo {
        VkBuffer vkBuffer = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo = {};
    };
    
    BufferInfo* CreateBuffer(size_t size, VkBufferUsageFlags usage);
    void DestroyBuffer(BufferInfo* buffer);
   
private:
    void _InitVkInstance();
    void _InitVkSurfaceKHR();
    void _InitVKDevice();
    void _InitVMAAllocator();
    void _InitVkCommandPool();
    void _InitVkDescriptorPool();
   
private:
    Window *window = VK_NULL_HANDLE;
    
    uint32_t apiVersion = 0;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t queueIndex = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};