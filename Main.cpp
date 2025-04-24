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

int main()
{
    system("chcp 65001 >nul");

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    std::unique_ptr<Window> window = std::make_unique<Window>(800, 600, "NATURE");
    RenderDevice *device = RenderDevice::Create(window.get(), RENDER_API_FOR_VULKAN);

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
        .shaderModules = {
            {ShaderStageFlags::Vertex,   "SimpleShader.vert", true},
            {ShaderStageFlags::Fragment, "SimpleShader.frag", true},
        },
        .layout = {
            .descriptorBindings = {
                {.set = 0, .binding = 0, .count = 1, .stages = ShaderStageFlags::Vertex, .type = DescriptorType::Sampler}
            },
        },
        .assemblyState = AssemblyState::Default(),
        .rasterState = RasterState::Default(),
        .depthState = DepthState::Enabled(),
        .blendState = BlendState::Disabled(),
    };

    Pipeline* pipeline = device->CreatePipeline(&pipelineCreateInfo);

    device->DestroyPipeline(pipeline);
    RenderDevice::Destroy(device);

}