#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(set = 0, bind = 0) sampler2D inTexture;

layout(location = 0) out vec4 fragColor;

void main()
{
    // fragColor = vec4(abs(normalize(inNormal)), 1.0f);
    fragColor = texture(inTexture, inTexCoord);
}