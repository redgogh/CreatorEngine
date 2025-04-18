#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = inNormal;
    
    if (length(color) < 1.0f)
        color = vec3(0.5, 0.5, 1.0f);
    
    outColor = vec4(color, 1.0f);
}