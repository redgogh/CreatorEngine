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
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanSampler.h"
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

    std::unordered_map<uint32_t, DescriptorSetLayoutInfo> descriptorSetLayoutInfos;
    
    for (const auto &descriptorBinding: pPipelineCreateInfo->layout.descriptorBindings) {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
        descriptorSetLayoutBinding.binding = descriptorBinding.binding;
        descriptorSetLayoutBinding.descriptorType = ToVkDescriptorType(descriptorBinding.type);
        descriptorSetLayoutBinding.descriptorCount = descriptorBinding.count;
        descriptorSetLayoutBinding.stageFlags = ToVkShaderStageFlagBits(descriptorBinding.stages);
        descriptorSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

        DescriptorSetLayoutInfo& descriptorSetLayoutInfo = descriptorSetLayoutInfos[descriptorBinding.set];
        descriptorSetLayoutInfo.descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
    }

    for (const auto &pair: descriptorSetLayoutInfos) {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(std::size(pair.second.descriptorSetLayoutBindings)),
            .pBindings = std::data(pair.second.descriptorSetLayoutBindings)
        };

        VkDescriptorSetLayout descriptorSetLayout;
        vkCreateDescriptorSetLayout(device->GetDevice(), &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &descriptorSetLayout);
        descriptorSetLayouts[pair.second.setIndex] = descriptorSetLayout;   
    }
    
    /////////////////////////////////////////////
    ///        Pipeline Layout Binding        ///
    /////////////////////////////////////////////
    
    std::vector<VkPushConstantRange> pushConstantRanges;

    for (const auto &range: pPipelineCreateInfo->layout.pushConstantRanges) {
        pushConstantRanges.push_back({
            .stageFlags = ToVkShaderStageFlagBits(range.stageFlags),
            .offset = range.offset,
            .size = range.size
        });
    }
    
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
    for (auto& [_, value] : descriptorSetLayouts) {
        _descriptorSetLayouts.push_back(value);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(std::size(_descriptorSetLayouts)),
        .pSetLayouts = std::data(_descriptorSetLayouts),
        .pushConstantRangeCount = static_cast<uint32_t>(std::size(pushConstantRanges)),
        .pPushConstantRanges = std::data(pushConstantRanges),
    };

    vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout);
    
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
    ///         Viewport State Create         ///
    /////////////////////////////////////////////

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
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
    ///          Multisample Create           ///
    /////////////////////////////////////////////
    
    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = ToVkSampleCount(pPipelineCreateInfo->multisampleState.sampleCount),
        .sampleShadingEnable = pPipelineCreateInfo->multisampleState.enableSampleShading ? VK_TRUE : VK_FALSE,
        
    };

    /////////////////////////////////////////////
    ///         DepthStencil Create           ///
    /////////////////////////////////////////////
    
    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = pPipelineCreateInfo->depthState.depthTest ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = pPipelineCreateInfo->depthState.depthWrite ? VK_TRUE : VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = pPipelineCreateInfo->depthState.minDepth,
        .maxDepthBounds = pPipelineCreateInfo->depthState.maxDepth,
    };

    /////////////////////////////////////////////
    ///         Color Blend Create           ///
    /////////////////////////////////////////////
    
    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
        .blendEnable = pPipelineCreateInfo->blendState.enabled ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };
    
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &pipelineColorBlendAttachmentState,
    };

    /////////////////////////////////////////////
    ///         Dynamic State Create          ///
    /////////////////////////////////////////////

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = std::size(dynamicStates),
        .pDynamicStates = std::data(dynamicStates),
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .stageCount = static_cast<uint32_t>(std::size(pipelineShaderStageCreateInfos)),
        .pStages = std::data(pipelineShaderStageCreateInfos),
        .pVertexInputState = &pipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
        .pViewportState = &pipelineViewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = &pipelineMultisampleStateCreateInfo,
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &pipelineColorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = pipelineLayout,
    };
    
    vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &pipeline);
    
    /* TODO 删除 ShaderModule */
    
}

VulkanPipeline::~VulkanPipeline()
{
    for (const auto &pair: descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(device->GetDevice(), pair.second, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, VK_NULL_HANDLE);
    vkDestroyPipeline(device->GetDevice(), pipeline, VK_NULL_HANDLE);
}

void VulkanPipeline::UpdateBinding(const Pipeline::BindingData data)
{
    DescriptorSetInfo& descriptorSetInfo = descriptorSets[data.set];
    
    if (!descriptorSetInfo.allocated) {
        device->AllocateDescriptorSet(descriptorSetLayouts[data.set], &descriptorSetInfo.descriptorSet);
        descriptorSetInfo.layout = descriptorSetLayouts[data.set];
        descriptorSetInfo.allocated = true;
    }
    
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSetInfo.descriptorSet,
        .dstBinding = data.binding,
        .descriptorCount = 1,
        .descriptorType = ToVkDescriptorType(data.type),
    };
    
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    VkDescriptorImageInfo descriptorImageInfo = {};
    
    switch (data.type) {
        case DescriptorType::UniformBuffer:
        case DescriptorType::StorageBuffer: {
            VulkanBuffer* vulkanBuffer = (VulkanBuffer*) data.buffer;
            descriptorBufferInfo.buffer = vulkanBuffer->GetVkBuffer();
            descriptorBufferInfo.offset = data.offset;
            descriptorBufferInfo.range = data.range <= 0 ? VK_WHOLE_SIZE : data.range;
            write.pBufferInfo = &descriptorBufferInfo;
            break;
        }

        case DescriptorType::SamplerImage:
        case DescriptorType::StorageImage: {
            VulkanTexture* vulkanTexture = (VulkanTexture*) data.image;
            descriptorImageInfo.imageView = vulkanTexture->GetVkImageView();
            write.pImageInfo = &descriptorImageInfo;
            break;
        }
        
        case DescriptorType::Sampler: {
            VulkanSampler* vulkanSampler = (VulkanSampler*) data.image;
            descriptorImageInfo.sampler = vulkanSampler->GetVkSampler();
            break;
        }
    }
    
    vkUpdateDescriptorSets(device->GetDevice(), 1, &write, 0, VK_NULL_HANDLE);
}