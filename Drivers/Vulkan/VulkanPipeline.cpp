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
#include "VulkanPipeline.h"

#include "Utils/IOUtils.h"

VulkanPipeline::VulkanPipeline(const VulkanDevice* _device, const PipelineCreateInfo* pPipelineCreateInfo) : device(_device)
{
    /////////////////////////////////////////////
    ///           Shader Module               ///
    /////////////////////////////////////////////

    std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

    for (const auto &shaderInfo: pPipelineCreateInfo->shaderInfos) {
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        size_t size = 0;
        char* buf = IOUtils::ReadByteBuf(shaderInfo.pShader, &size);

        VkShaderModuleCreateInfo shaderModuleCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = (uint32_t*) buf,
        };

        vkCreateShaderModule(device->GetDevice(), &shaderModuleCreateInfo, VK_NULL_HANDLE, &shaderModule);

        pipelineShaderStageCreateInfos.push_back({
           .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .stage = ToVkShaderStageFlagBits(shaderInfo.stage),
           .module = shaderModule,
           .pName = "main",
        });

        IOUtils::FreeByteBuf(buf);
    }

    /////////////////////////////////////////////
    ///        Descriptor Set Layout          ///
    /////////////////////////////////////////////

    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    
    for (const auto &descriptorBinding: pPipelineCreateInfo->layout.descriptorBindings) {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
        descriptorSetLayoutBinding.binding = descriptorBinding.binding;
        descriptorSetLayoutBinding.descriptorType = ToVkDescriptorType(descriptorBinding.type);
        descriptorSetLayoutBinding.descriptorCount = descriptorBinding.count;
        descriptorSetLayoutBinding.stageFlags = ToVkShaderStageFlagBits(descriptorBinding.stages);
        descriptorSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;
        descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
    }
    
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(std::size(descriptorSetLayoutBindings)),
        .pBindings = std::data(descriptorSetLayoutBindings)
    };
    
    vkCreateDescriptorSetLayout(device->GetDevice(), &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &descriptorSetLayout);

    /////////////////////////////////////////////
    ///         Vertex Input Binding          ///
    /////////////////////////////////////////////

    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescription;

    for (const auto &vertexBinding: pPipelineCreateInfo->vertexBindings) {
        vertexInputBindingDescriptions.push_back({
            .binding = vertexBinding.binding,
            .stride = vertexBinding.stride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        });

        for (const auto &attri: vertexBinding.attributes) {
            vertexInputAttributeDescription.push_back({
                .location = attri.location,
                .binding = vertexBinding.binding,
                .format = ToVkFormat(attri.format),
                .offset = attri.offset,
            });
        }
    }

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(std::size(vertexInputBindingDescriptions)),
        .pVertexBindingDescriptions = std::data(vertexInputBindingDescriptions),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(std::size(vertexInputAttributeDescription)),
        .pVertexAttributeDescriptions = std::data(vertexInputAttributeDescription),
    };
    
    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = ToVkTopology(pPipelineCreateInfo->assemblyState.topology)
    };

    /////////////////////////////////////////////
    ///        Rasterization Create           ///
    /////////////////////////////////////////////

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = ToVkPolygonMode(pPipelineCreateInfo->rasterState.polygonMode),
        .cullMode = ToVkCullMode(pPipelineCreateInfo->rasterState.cullMode),
    };

    /////////////////////////////////////////////
    ///            Pipeline Create            ///
    /////////////////////////////////////////////

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .stageCount = std::size(pipelineShaderStageCreateInfos),
        .pStages = std::data(pipelineShaderStageCreateInfos),
        .pVertexInputState = &pipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
        .pViewportState = &pipelineViewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = &pipelineMultisampleStateCreateInfo,
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &pipelineColorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = &pipelineLayout,
    };
    
}

VulkanPipeline::~VulkanPipeline()
{
    
}