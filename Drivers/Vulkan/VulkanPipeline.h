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

#include "Drivers/Pipeline.h"

#include "VulkanDevice.h"

class VulkanPipeline : public Pipeline {
public:
    VulkanPipeline(const VulkanDevice* _device, const PipelineCreateInfo* pPipelineCreateInfo);
    virtual ~VulkanPipeline() override;

    VkPipeline GetVkPipeline() const { return pipeline; }
    VkPipelineLayout GetVkPipelineLayout() const { return pipelineLayout; }

private:
    static VkShaderStageFlagBits ToVkShaderStageFlagBits(ShaderStageFlags stage)
    {
        switch(stage) {
            case ShaderStageFlags::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStageFlags::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            default: throw std::runtime_error("[Vulkan] Unsupported Shader stage flag");
        }
    }

    VkDescriptorType ToVkDescriptorType(DescriptorType type) {
        switch (type) {
            case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            default: throw std::runtime_error("[Vulkan] Unsupported DescriptorType");
        }
    }
    
    VkFormat ToVkFormat(VertexFormat format)
    {
        switch (format) {
            case VertexFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
            case VertexFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
            case VertexFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: throw std::runtime_error("[Vulkan] Unsupported VertexFormat");
        }
    }

    VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology)
    {
        switch (topology) {
            case PrimitiveTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case PrimitiveTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveTopology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::TriangleFan:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            default: throw std::runtime_error("[Vulkan] Unsupported topology");
        }
    }

    VkPolygonMode ToVkPolygonMode(PolygonMode polygonMode)
    {
        switch (polygonMode) {
            case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
            case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
            case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
            default: throw std::runtime_error("[Vulkan] Unsupported polygon mode");
        }
    }

    VkCullModeFlagBits ToVkCullMode(CullMode cullMode)
    {
        switch (cullMode) {
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
            default: throw std::runtime_error("[Vulkan] Unsupported cull mode");
        }
    }
    
private:
    const VulkanDevice* device = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
};