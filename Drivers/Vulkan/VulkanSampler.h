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

#include "Drivers/Sampler.h"
#include "VulkanDevice.h"

class VulkanSampler : public Sampler {
public:
    VulkanSampler(const VulkanDevice* _device, SamplerCreateInfo* pSamplerCreateInfo);
    virtual ~VulkanSampler() override;
    
    VkSampler GetVkSampler() const { return sampler; }

private:
    static VkFilter ToVkFilter(SamplerFilterFlags filter)
    {
        switch (filter) {
            case SamplerFilterFlags::Linear: return VK_FILTER_LINEAR;
            case SamplerFilterFlags::Nearest: return VK_FILTER_NEAREST;
            default: throw std::runtime_error("[Vulkan] Unsupported filter");
        }
    }

    static VkSamplerAddressMode ToVkSamplerAddressMode(SamplerAddressMode mode) {
        switch (mode) {
            case SamplerAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerAddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case SamplerAddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerAddressMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case SamplerAddressMode::MirrorClampToEdge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            default: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }
    }


private:
    const VulkanDevice* device = VK_NULL_HANDLE;

    VkSampler sampler = VK_NULL_HANDLE;
};