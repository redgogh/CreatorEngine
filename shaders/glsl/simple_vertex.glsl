#version 450

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 out_color;

layout(push_constant) uniform push_const {
    mat4 mvp;
} upc;

void main()
{
    gl_Position = upc.mvp * vec4(vertex, 1.0f);

    out_color = vec3(0.5f, 0.1f, 1.0f);
    
}