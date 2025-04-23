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

#include "Drivers/RenderDevice.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandList.h"

class VulkanRenderDevice : public RenderDevice {
public:
    VulkanRenderDevice(const Window* _window);
    virtual ~VulkanRenderDevice() override;

    virtual Buffer* CreateBuffer(size_t size, BufferUsageFlags usage) override final;
    virtual void DestroyBuffer(Buffer* buffer) override final;
    virtual SwapChain* CreateSwapChain() override final;
    virtual void DestroySwapChain(SwapChain* swapchain) override final;
    virtual CommandList* CreateCommandList() override final;
    virtual void DestroyCommandList(CommandList* commandList) override final;
    virtual Pipeline* CreatePipeline(const PipelineCreateInfo* pPipelineCreateInfo) override final;
    virtual void DestroyPipeline(Pipeline* pipeline) override final;

private:
    VulkanContext* vkContext = VK_NULL_HANDLE;
    VulkanDevice* vkDevice = VK_NULL_HANDLE;

};
