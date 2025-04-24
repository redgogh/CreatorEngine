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
#include "VulkanTexture.h"

VulkanTexture::VulkanTexture(const VulkanDevice* _device, uint32_t w, uint32_t h, Texture::Format format, Texture::Usage usage)
    : device(_device), width(w), height(h)
{
    this->format = ToVkFormat(format);
    
    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = this->format,
        .extent = {w, h, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = ToVkImageUsage(usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    vmaCreateImage(device->GetAllocator(), &imageCreateInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo);
    device->CreateImageView(image, this->format, &imageView);
}

VulkanTexture::~VulkanTexture()
{
    vmaDestroyImage(device->GetAllocator(), image, allocation);
    device->DestroyImageView(imageView);
}

void VulkanTexture::Upload(const void *data, size_t size)
{
    
}

void VulkanTexture::ReadBack(void *dst, size_t size)
{
    
}