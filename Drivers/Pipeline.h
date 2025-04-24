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

enum class ShaderStageFlags
{
    Vertex, // = 1 << 0,
    Fragment, // = 1 << 1,
    Geometry, // = 1 << 2,
    TessControl, // = 1 << 3,
    TessEval, // = 1 << 4,
    Mesh, // = 1 << 5,
    Task, // = 1 << 6,
};

enum class CullMode 
{ 
    None, 
    Back, 
    Front,
    FrontAndBack,
};

enum class PolygonMode
{
    Fill, 
    Line, 
    Point
};

enum class PrimitiveTopology 
{ 
    PointList, 
    LineList, 
    LineStrip, 
    TriangleList, 
    TriangleStrip, 
    TriangleFan 
};

enum class DescriptorType
{
    UniformBuffer,
    StorageBuffer,
    Sampler,
    SamplerImage,
    StorageImage,
};

enum class VertexFormat
{
    Float2,
    Float3,
    Float4,
};

enum class SampleCount
{
    X0  =  0,
    X1  =  1,
    X2  =  2,
    X4  =  4,
    X8  =  8,
    X16 = 16,
};

struct VertexAttribute
{
    uint32_t location;
    uint32_t offset;
    VertexFormat format;
};

struct VertexBinding
{
    uint32_t binding;
    uint32_t stride;
    std::vector<VertexAttribute> attributes;
};

struct DescriptorBinding
{
    uint32_t set;
    uint32_t binding;
    uint32_t count;
    ShaderStageFlags stages;
    DescriptorType type;
};

/** 着色器模块 */
struct ShaderInfo
{
    ShaderStageFlags stage;
    const char* pShader;
    // 如果 enableHotReload 启用则会运行时动态热加载着色器
    bool enableHotReload; 
};

/** 深度测试 */
struct DepthState
{
    bool depthTest;
    bool depthWrite;
    bool depthClamp;
    float minDepth;
    float maxDepth;
    
    // -- Wrap --
    static DepthState Enabled() { return { true, true, false, 0.0f, 1.0f }; }
    static DepthState Disabled() { return { false, false, false, 0.0f, 1.0f }; }
    
};

/** 颜色混合 */
struct BlendState
{
    bool enabled;

    // -- Wrap --
    static BlendState Enabled() { return { true }; }
    static BlendState Disabled() { return { false }; }
    
};

/** 光栅化阶段 */
struct RasterState
{
    CullMode cullMode;
    PolygonMode polygonMode;
    bool frontCCW;
    
    // -- Wrap --
    static RasterState Default() { return { CullMode::Back, PolygonMode::Fill, true }; }
    
};

struct AssemblyState
{
    PrimitiveTopology topology;
    bool primitiveRestartEnable;

    // -- Wrap --
    static AssemblyState Default() { return { PrimitiveTopology::TriangleList, false }; }
    
};

/** 多重采样 */
struct MultisampleState
{
    SampleCount sampleCount;
    bool enableSampleShading;

    // -- Wrap --
    static MultisampleState Disabled() { return { SampleCount::X0, false }; }
    static MultisampleState MSAA4x()   { return { SampleCount::X4, false }; }
    static MultisampleState MSAA8x()   { return { SampleCount::X8, false }; }
    
};

struct PushConstantRange
{
    ShaderStageFlags stageFlags;
    uint32_t offset;
    uint32_t size;
};

/** 管线布局 */
struct PipelineLayout
{
    std::vector<DescriptorBinding> descriptorBindings;
    std::vector<PushConstantRange> pushConstantRanges;
};

struct PipelineCreateInfo {
    std::vector<VertexBinding> vertexBindings;
    std::vector<ShaderInfo> shaderInfos;
    PipelineLayout layout;
    AssemblyState assemblyState;
    RasterState rasterState;
    DepthState depthState;
    BlendState blendState;
    MultisampleState multisampleState;
};

class Pipeline {
public:
    virtual ~Pipeline() = default;

    struct BindingData {
        uint32_t set;
        uint32_t binding;
        DescriptorType type;
        union {
            void* buffer;
            void* image;
            void* sampler;
        };
        uint32_t offset = 0;
        uint32_t range = 0;
    };
    
    virtual void UpdateBinding(const BindingData data) = 0;
    
};