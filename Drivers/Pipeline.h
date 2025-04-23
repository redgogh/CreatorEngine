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

enum CullMode
{
    None,
    Back,
    Front,
};

enum FillMode
{
    Fill,
    Line,
    Point,
};

enum PrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
};

struct RasterState
{
    CullMode cullMode;
    FillMode fillMode;
    bool frontCounterClockwise;

    static RasterState Default() {
        return {
            .cullMode = CullMode::Back,
            .fillMode = FillMode::Fill,
            .frontCounterClockwise = true,
        };
    }
};

struct PipelineCreateInfo
{
    RasterState* pRasterState;
    PrimitiveTopology topology;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;
};