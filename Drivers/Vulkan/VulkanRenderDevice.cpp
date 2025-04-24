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
#include "VulkanRenderDevice.h"

VulkanRenderDevice::VulkanRenderDevice(const Window* _window)
{
    context = MemoryNew<VulkanContext>(_window);
    device = MemoryNew<VulkanDevice>(context);
}

VulkanRenderDevice::~VulkanRenderDevice()
{
    MemoryDelete(device);
    MemoryDelete(context);
}

Buffer *VulkanRenderDevice::CreateBuffer(size_t size, BufferUsageFlags usage)
{
    return MemoryNew<VulkanBuffer>(device, size, usage);
}

void VulkanRenderDevice::DestroyBuffer(Buffer *buffer)
{
    return MemoryDelete(buffer);
}

SwapChain* VulkanRenderDevice::CreateSwapChain()
{
    return MemoryNew<VulkanSwapChain>(context, device);
}

void VulkanRenderDevice::DestroySwapChain(SwapChain* swapchain)
{
    MemoryDelete(swapchain);
}

CommandList *VulkanRenderDevice::CreateCommandList()
{
    return MemoryNew<VulkanCommandList>(device);
}

void VulkanRenderDevice::DestroyCommandList(CommandList *commandList)
{
    MemoryDelete(commandList);
}

Pipeline *VulkanRenderDevice::CreatePipeline(const PipelineCreateInfo* pPipelineCreateInfo)
{
    return MemoryNew<VulkanPipeline>(device, pPipelineCreateInfo);
}

void VulkanRenderDevice::DestroyPipeline(Pipeline *pipeline)
{
    MemoryDelete(pipeline);
}