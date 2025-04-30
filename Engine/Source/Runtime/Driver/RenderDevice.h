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

#include "VulkanInclude.h"
#include "Window/Window.h"

#include "CommandList.h"
#include "Buffer.h"

#include <Vector.h>

class RenderDevice
{
public:
    RenderDevice(Window* pWindow);
   ~RenderDevice();
    
    Buffer* CreateBuffer(size_t size, VkBufferUsageFlags usage);
    void DestroyBuffer(Buffer* buffer);
    CommandList* CreateCommandList();
    void DestroyCommandLis(CommandList* commandList);
    
    struct SwapchainVkEXT {
        VkSwapchainKHR vkSwapchainKHR = VK_NULL_HANDLE;
        VkSurfaceCapabilitiesKHR capabilities = {};
        uint32_t minImageCount = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        struct SwapchainResourceVkEXT {
            VkImage image = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
        };
        Vector<SwapchainResourceVkEXT> resources;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t acquireIndex = 0;
        uint32_t frame = 0;
        float aspect = 0.0f;
        std::vector<VkSemaphore> acquireIndexSemaphore;
        std::vector<VkSemaphore> renderFinishSemaphore;
        std::vector<VkFence> fence;
        std::vector<CommandList*> commandLists;
    };

    SwapchainVkEXT* CreateSwapchainEXT(SwapchainVkEXT* oldSwapchainEXT);
    void DestroySwapchainEXT(SwapchainVkEXT* swapchain);

private:
    VkResult _CreateImageView(VkImage image, VkFormat formamt, VkImageView* pImageView);
    void _DestroyImageView(VkImageView imageView);
    VkResult _CreateSemaphore(VkSemaphore* pSemaphore);
    void _DestroySemaphore(VkSemaphore semaphore);
    VkResult _CreateFence(VkFence* pFence);
    void _DestroyFence(VkFence fence);
    
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
    uint32_t queueFamilyIndex = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};