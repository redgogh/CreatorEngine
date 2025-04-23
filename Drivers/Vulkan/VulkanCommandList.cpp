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
#include "VulkanCommandList.h"

VulkanCommandList::VulkanCommandList(const VulkanDevice* _device) : device(_device)
{
    device->AllocateCommandBuffer(&commandBuffer);
}

VulkanCommandList::~VulkanCommandList()
{
    device->FreeCommandBuffer(commandBuffer);
}

void VulkanCommandList::Begin()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
}

void VulkanCommandList::End()
{
    vkEndCommandBuffer(commandBuffer);
}


void VulkanCommandList::CmdBindPipeline(Pipeline* pipeline)
{

}

void VulkanCommandList::CmdBindVertexBuffer(Buffer* buffer, uint32_t offset)
{
    VulkanBuffer* vkBuffer = dynamic_cast<VulkanBuffer*>(buffer);
    VkBuffer buffers[] = { vkBuffer->GetHandle() };
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void VulkanCommandList::CmdBindIndexBuffer(Buffer* buffer, uint32_t offset, uint32_t indexCount)
{
    VulkanBuffer* vkBuffer = dynamic_cast<VulkanBuffer*>(buffer);
    vkCmdBindIndexBuffer(commandBuffer, vkBuffer->GetHandle(), offset, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandList::CmdDraw(uint32_t vertexCount)
{
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void VulkanCommandList::CmdDrawIndexed(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset)
{
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);

}

void VulkanCommandList::Reset()
{
    vkResetCommandBuffer(commandBuffer, 0);
}