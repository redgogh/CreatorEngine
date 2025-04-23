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

#include "Drivers/CommandList.h"

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

class VulkanCommandList : public CommandList
{
public:
    VulkanCommandList(const VulkanDevice* v_Device);
    virtual ~VulkanCommandList() override;

    virtual void Begin() override final;
    virtual void End() override final;

    virtual void CmdBindPipeline(Pipeline* pipeline) override final;
    virtual void CmdBindVertexBuffer(Buffer* buffer, uint32_t offset) override final;
    virtual void CmdBindIndexBuffer(Buffer* buffer, uint32_t offset, uint32_t indexCount) override final;
    virtual void CmdDraw(uint32_t vertexCount) override final;
    virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) override final;

    virtual void Reset() override final;

private:
    const VulkanDevice* device = VK_NULL_HANDLE;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

};