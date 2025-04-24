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

#include "VulkanContext.h"

class VulkanDevice {
public:
    VulkanDevice(const VulkanContext* _ctx);
   ~VulkanDevice();
   
   VkDevice GetDevice() const { return device; }
   VmaAllocator GetAllocator() const { return allocator; }
   void GetSurfaceCapabilities(VkSurfaceCapabilitiesKHR* pSurfaceCapabilitiesKHR) const;
   VkResult PickSurfaceFormat(VkSurfaceFormatKHR *pSurfaceFormat) const;

   VkResult CreateFence(VkFence *pFence) const;
   void DestroyFence(VkFence fence) const;
   VkResult CreateSemaphoreVk(VkSemaphore *pSemaphore) const;
   void DestroySemaphoreVk(VkSemaphore semaphore) const;
   VkResult CreateImageView(VkImage image, VkFormat format, VkImageView *pImageView) const;
   void DestroyImageView(VkImageView imageView) const;
   VkResult AllocateCommandBuffer(VkCommandBuffer *pCommandBuffer) const;
   void FreeCommandBuffer(VkCommandBuffer commandBuffer) const;
   VkResult AllocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet* pDescriptorSet) const;
   void FreeDescriptorSet(VkDescriptorSet descriptorSet) const;

private:
    const VulkanContext* vkContext = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t queueIndex = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    
};