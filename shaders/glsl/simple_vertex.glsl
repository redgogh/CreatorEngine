#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

layout(push_constant) uniform PushConst {
    mat4 mvp;
} upc;

void main()
{
    gl_Position = upc.mvp * vec4(inPos, 1.0f);

    fragPos = inPos;
    fragTexCoord = inTexCoord;
    fragNormal = inNormal;
}