/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 RedGogh All rights reserved.                          *|
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

#include <Vdx/Typedef.h>

#include "VdxBuffer.h"

enum VdxRenderAPI {
    VDX_RENDER_API_FOR_VULKAN,
};

enum VdxBufferUsageFlagBits {
    VDX_BUFFER_USAGE_VERTEX_BUFFER_BIT = 1 << 0,
    VDX_BUFFER_USAGE_INDEX_BUFFER_BIT = 1 << 1,
};

class VdxRenderDevice {
public:
    virtual ~VdxRenderDevice() = default;
    
    virtual VdxBuffer* CreateBuffer(size_t size, VdxBufferUsageFlagBits usage) = 0;
    virtual void DestroyBuffer(VdxBuffer* buffer) = 0;
    
public:
    static VdxRenderDevice* Create(VdxRenderAPI api);
    static void Destroy(VdxRenderDevice* device);
};