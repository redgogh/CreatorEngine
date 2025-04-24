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
#include "Drivers/RenderDevice.h"

float vertices[] = {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
     0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
};

uint32_t indices[] = {
    0, 1, 2,
    2, 3, 0,
};

int main()
{
    system("chcp 65001 >nul");

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    Window* window;
    RenderDevice *device;
    Pipeline *pipeline;
    CommandList* commandList;
    Buffer* vertexBuffer;
    Buffer* indexBuffer;

    window = MemoryNew<Window>(800, 600, "NATURE");
    device = RenderDevice::Create(window, RENDER_API_FOR_VULKAN);
    commandList = device->CreateCommandList();

    PipelineCreateInfo pipelineCreateInfo = {
        .vertexBindings = {
            {
                .binding = 0,
                .stride = 32,
                .attributes = {
                    {.location = 0, .offset =  0, .format = VertexFormat::Float3},
                    {.location = 1, .offset = 12, .format = VertexFormat::Float2},
                    {.location = 2, .offset = 20, .format = VertexFormat::Float3},
                }
            }
        },
        .shaderInfos = {
            {.stage = ShaderStageFlags::Vertex, .pShader = "SimpleShader.vert", .enableHotReload = true},
            {.stage = ShaderStageFlags::Fragment, .pShader = "SimpleShader.frag", .enableHotReload = true},
        },
        .layout = {
            .descriptorBindings = {
                {.set = 0, .binding = 0, .count = 1, .stages = ShaderStageFlags::Vertex, .type = DescriptorType::Sampler}
            },
            .pushConstantRanges = {
                {ShaderStageFlags::Vertex, 0, 32}
            }
        },
        .assemblyState = AssemblyState::Default(),
        .rasterState = RasterState::Default(),
        .depthState = DepthState::Enabled(),
        .blendState = BlendState::Disabled(),
        .multisampleState = MultisampleState::MSAA4x()
    };

    pipeline = device->CreatePipeline(&pipelineCreateInfo);

    vertexBuffer = device->CreateBuffer(sizeof(vertices), BufferUsageFlags::Vertex);
    indexBuffer = device->CreateBuffer(sizeof(indices), BufferUsageFlags::Index);
    
    vertexBuffer->Upload(0, sizeof(vertices), vertices);
    indexBuffer->Upload(0, sizeof(indices), indices);
    
    // 录制指令
    commandList->Begin();
    commandList->CmdBindPipeline(pipeline);
    commandList->CmdBindVertexBuffer(vertexBuffer, 0);
    commandList->CmdBindIndexBuffer(indexBuffer, 0, sizeof(indices) / sizeof(uint32_t));
    commandList->CmdDrawIndexed(sizeof(indices) / sizeof(uint32_t), 0, 0);
    commandList->End();

    // 执行并复用 Command Buffer
    commandList->Execute();
    commandList->Reset();
    
    device->DestroyPipeline(pipeline);
    RenderDevice::Destroy(device);
    MemoryDelete(window);
    
}