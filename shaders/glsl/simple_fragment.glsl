#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(abs(normalize(inNormal)), 1.0f);
}