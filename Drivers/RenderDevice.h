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

#include "Window/Window.h"
#include "Buffer.h"
#include "SwapChain.h"
#include "CommandList.h"
#include "Pipeline.h"
#include "Sampler.h"
#include "Texture.h"

enum RenderAPI {
    RENDER_API_FOR_VULKAN,
};

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual Buffer* CreateBuffer(size_t size, BufferUsageFlags usage) = 0;
    virtual void DestroyBuffer(Buffer* buffer) = 0;
    virtual SwapChain* CreateSwapChain() = 0;
    virtual void DestroySwapChain(SwapChain* swpachain) = 0;
    virtual CommandList* CreateCommandList() = 0;
    virtual void DestroyCommandList(CommandList* commandList) = 0;
    virtual Pipeline* CreatePipeline(const PipelineCreateInfo* pPipelineCreateInfo) = 0;
    virtual void DestroyPipeline(Pipeline* pipeline) = 0;
    virtual Sampler* CreateSampler(SamplerCreateInfo* pSamplerCreateInfo) = 0;
    virtual void DestroySampler(Sampler* sampler) = 0;
    virtual Texture* CreateTexture(uint32_t w, uint32_t h, Texture::Format format, Texture::Usage usage) = 0;
    virtual void DestroyTexture(Texture* texture) = 0;

public:
    static RenderDevice* Create(const Window* window, const RenderAPI& renderAPI);
    static void Destroy(RenderDevice* device);

};
