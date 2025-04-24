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

#include <Vdx/Typedef.h>

class Texture {
public:
    enum Format
    {
        RGBA8_UNorm,
        RGBA8_SRGB,
        BGRA8_UNorm,
        BGRA8_SRGB,
        RGB10A2_UNorm,
        R11G11B10_Float,
        RGBA16_Float,
        RGBA32_Float,
        
        Depth32_Float,
        Depth24_Stencil8,
        Depth32_Float_Stencil8,

        R8_UNorm,
        R16_Float,
        R32_Float,
        RG8_UNorm,
        RG16_Float,
        RG32_Float,

        R32_UInt,
        RG32_UInt,
        RGBA32_UInt
    };

    enum Usage
    {
        Usage_Sampled = 1 << 0,
        Usage_RenderTarget = 1 << 1,
        Usage_DepthStencil = 1 << 2,
        Usage_Storage = 1 << 3,
    };
    
public:
    virtual ~Texture() = default;

    virtual size_t GetWidth() = 0;
    virtual size_t GetHeight() = 0;

    virtual void Upload(const void* data, size_t size) = 0;
    virtual void ReadBack(void* dst, size_t size) = 0;
    
};