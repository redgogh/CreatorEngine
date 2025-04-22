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
#include "Driver/RenderDevice.h"

int main()
{
    system("chcp 65001 >nul");

    /*
     * close stdout and stderr write to buf, let direct
     * output.
     */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    std::unique_ptr<Window> window = std::make_unique<Window>(800, 600, "NATURE");
    RenderDevice* device = RenderDevice::Create(window.get(), RENDER_API_FOR_VULKAN);

    Buffer* vertexBuffer = device->CreateBuffer(1024 * 4, BUFFER_USAGE_VERTEX_BIT);

    const char text[] = "hello world";
    vertexBuffer->WriteMemory(0, sizeof(text), text);

    char buf[32] = {0};
    vertexBuffer->ReadMemory(0, sizeof(text), buf);

    printf("%s\n", buf);

    device->DestroyBuffer(vertexBuffer);

    while (!window->IsShouldClose()) {
        window->PollEvents();
    }

    RenderDevice::Destroy(device);
}