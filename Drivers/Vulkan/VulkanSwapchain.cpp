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
#include "VulkanSwapChain.h"

VulkanSwapChain::VulkanSwapChain(const VulkanContext *_ctx, const VulkanDevice *_device)
    : vkContext(_ctx), vkDevice(_device)
{
    vkDevice->GetSurfaceCapabilities(&capabilities);
    CreateSwapChainKHR(capabilities.currentExtent.width, capabilities.currentExtent.height);
}

VulkanSwapChain::~VulkanSwapChain()
{
    DestroySwapChainResourceKHR();
    vkDestroySwapchainKHR(vkDevice->GetDevice(), swapchain, VK_NULL_HANDLE);
}

void VulkanSwapChain::CreateSwapChainKHR(uint32_t w, uint32_t h)
{
    VkResult err;

    vkDevice->GetSurfaceCapabilities(&capabilities);

    uint32_t min = capabilities.minImageCount;
    uint32_t max = capabilities.maxImageCount;
    minImageCount = std::clamp(min + 1, min, max);

    width = w;
    height = h;
    aspectRatio = (float) width / height;

    VkSurfaceFormatKHR surfaceFormat;
    vkDevice->PickSurfaceFormat(&surfaceFormat);

    format = surfaceFormat.format;
    colorSpace = surfaceFormat.colorSpace;

    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkContext->GetSurfaceKHR(),
        .minImageCount = minImageCount,
        .imageFormat = format,
        .imageColorSpace = colorSpace,
        .imageExtent = capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = swapchain,
    };

    VkSwapchainKHR newSwapChain = VK_NULL_HANDLE;
    err = vkCreateSwapchainKHR(vkDevice->GetDevice(), &swapchainCreateInfoKHR, VK_NULL_HANDLE, &newSwapChain);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to create swap chain", err);
    
    if (!swapchain) {
        vkDestroySwapchainKHR(vkDevice->GetDevice(), swapchain, VK_NULL_HANDLE);
    }
    
    swapchain = newSwapChain;

    uint32_t count;
    err = vkGetSwapchainImagesKHR(vkDevice->GetDevice(), swapchain, &count, VK_NULL_HANDLE);
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to get image count in swap chain", err);

    std::vector<VkImage> images(count);
    err = vkGetSwapchainImagesKHR(vkDevice->GetDevice(), swapchain, &count, std::data(images));
    VK_CHECK_FATAL_ERROR("[Vulkan] Failed to get image in swap chain", err);

    resources.resize(minImageCount);
    for (int i = 0; i < minImageCount; ++i) {
        resources[i].image = images[i];
        vkDevice->CreateFence(&resources[i].fence);
        vkDevice->CreateSemaphoreVk(&resources[i].semaphore);
        vkDevice->CreateImageView(images[i], format, &resources[i].imageView);
    }
}

void VulkanSwapChain::DestroySwapChainResourceKHR()
{
    for (int i = 0; i < minImageCount; i++) {
        vkDevice->DestroyFence(resources[i].fence);
        vkDevice->DestroySemaphoreVk(resources[i].semaphore);
        vkDevice->DestroyImageView(resources[i].imageView);
    }
}

void VulkanSwapChain::Resize(uint32_t w, uint32_t h)
{
    DestroySwapChainResourceKHR();
    CreateSwapChainKHR(w, h);
}

uint32_t VulkanSwapChain::AcquireNextImage()
{
    vkAcquireNextImageKHR(vkDevice->GetDevice(), swapchain, UINT32_MAX, resources[acquireIndex].semaphore, VK_NULL_HANDLE, &acquireIndex);
    return acquireIndex;
}

void VulkanSwapChain::Present()
{

}