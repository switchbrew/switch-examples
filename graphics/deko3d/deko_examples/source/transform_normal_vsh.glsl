#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inAttrib;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outAttrib;

layout (std140, binding = 0) uniform Transformation
{
    mat4 mdlvMtx;
    mat4 projMtx;
} u;

void main()
{
    vec4 worldPos = u.mdlvMtx * vec4(inPos, 1.0);
    gl_Position = u.projMtx * worldPos;

    outWorldPos = worldPos.xyz;

    outNormal = normalize(mat3(u.mdlvMtx) * inNormal);

    // Pass through the user-defined attribute
    outAttrib = inAttrib;
}
