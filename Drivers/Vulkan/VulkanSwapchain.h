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
#pragma once

#include "Drivers/SwapChain.h"

#include "VulkanDevice.h"

class VulkanSwapChain : public SwapChain {
public:
    VulkanSwapChain(const VulkanContext* _ctx, const VulkanDevice* _device);
    virtual ~VulkanSwapChain() override;

    virtual uint32_t GetWidth() override { return width; }
    virtual uint32_t GetHeight() override { return height; }
    virtual float GetAspectRatio() override { return aspectRatio; }
    virtual uint32_t GetCurrentImageIndex() override { return acquireIndex; }
    
    virtual void Resize(uint32_t w, uint32_t h) override;
    virtual uint32_t AcquireNextImage() override;
    virtual void Present() override;
    
private:
    void CreateSwapChainKHR(uint32_t w, uint32_t h);
    void DestroySwapChainResourceKHR();
    
private:
    const VulkanContext* vkContext = VK_NULL_HANDLE;
    const VulkanDevice* vkDevice = VK_NULL_HANDLE;
    
    VkSurfaceCapabilitiesKHR capabilities = {};
    
    uint32_t width = 0;
    uint32_t height = 0;
    float aspectRatio = 0.0f;
    uint32_t minImageCount = 0;
    uint32_t acquireIndex = 0;
    
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
    
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    
    struct SwapChainResource {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore semaphore = VK_NULL_HANDLE;
    };
    
    std::vector<SwapChainResource> resources;
    
};