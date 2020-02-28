#version 460

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outViewDir;

void main()
{
    outAlbedo = vec4(1.0, 1.0, 1.0, 1.0);
    outNormal = vec4(normalize(inNormal), 0.0);
    outViewDir = vec4(-inWorldPos, 0.0);
}
