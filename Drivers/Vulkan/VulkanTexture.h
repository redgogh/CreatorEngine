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

#include "Drivers/Texture.h"
#include "VulkanDevice.h"

class VulkanTexture : public Texture {
public:
    VulkanTexture(const VulkanDevice* _device, uint32_t w, uint32_t h, Texture::Format format, Texture::Usage usage);
    virtual ~VulkanTexture() override;

    VkImage GetVkImage() const { return image; }
    VkImageView GetVkImageView() const { return imageView; }

    virtual size_t GetWidth() override final { return width; };
    virtual size_t GetHeight() override final { return height; };

    virtual void Upload(const void* data, size_t size) override final;
    virtual void ReadBack(void* dst, size_t size) override final;
    
private:
    static VkFormat ToVkFormat(Texture::Format format) {
        switch (format) {
            case Texture::Format::RGBA8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
            case Texture::Format::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case Texture::Format::BGRA8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
            case Texture::Format::BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case Texture::Format::RGB10A2_UNorm: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case Texture::Format::R11G11B10_Float: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case Texture::Format::RGBA16_Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case Texture::Format::RGBA32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;

            case Texture::Format::Depth32_Float: return VK_FORMAT_D32_SFLOAT;
            case Texture::Format::Depth24_Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
            case Texture::Format::Depth32_Float_Stencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;

            case Texture::Format::R8_UNorm: return VK_FORMAT_R8_UNORM;
            case Texture::Format::R16_Float: return VK_FORMAT_R16_SFLOAT;
            case Texture::Format::R32_Float: return VK_FORMAT_R32_SFLOAT;
            case Texture::Format::RG8_UNorm: return VK_FORMAT_R8G8_UNORM;
            case Texture::Format::RG16_Float: return VK_FORMAT_R16G16_SFLOAT;
            case Texture::Format::RG32_Float: return VK_FORMAT_R32G32_SFLOAT;

            case Texture::Format::R32_UInt: return VK_FORMAT_R32_UINT;
            case Texture::Format::RG32_UInt: return VK_FORMAT_R32G32_UINT;
            case Texture::Format::RGBA32_UInt: return VK_FORMAT_R32G32B32A32_UINT;

            default: return VK_FORMAT_UNDEFINED;
        }
    }

    static VkImageUsageFlags ToVkImageUsage(uint32_t usage) {
        VkImageUsageFlags flags = 0;

        if (usage & Usage_Sampled) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (usage & Usage_RenderTarget) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (usage & Usage_DepthStencil) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (usage & Usage_Storage) flags |= VK_IMAGE_USAGE_STORAGE_BIT;

        return flags;
    }


private:
    const VulkanDevice* device = VK_NULL_HANDLE;

    uint32_t width = 0;
    uint32_t height = 0;
    
    VkFormat format = VK_FORMAT_UNDEFINED;
    
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
};