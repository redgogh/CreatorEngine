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

#include "Buffer.h"
#include "Pipeline.h"

class CommandList
{
public:
    virtual ~CommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void CmdBindPipeline(Pipeline* pipeline) = 0;
    virtual void CmdSetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;
    virtual void CmdBindVertexBuffer(Buffer* buffer, uint32_t offset) = 0;
    virtual void CmdBindIndexBuffer(Buffer* buffer, uint32_t offset, uint32_t indexCount) = 0;
    virtual void CmdDraw(uint32_t vertexCount) = 0;
    virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) = 0;

    virtual void Execute() = 0;
    virtual void Reset() = 0;

};