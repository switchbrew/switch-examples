#version 460

layout (triangles, equal_spacing, ccw) in;

layout (location = 0) in vec4 inAttrib[];

layout (location = 0) out vec4 outAttrib;

vec4 interpolate(in vec4 v0, in vec4 v1, in vec4 v2)
{
    vec4 a0 = gl_TessCoord.x * v0;
    vec4 a1 = gl_TessCoord.y * v1;
    vec4 a2 = gl_TessCoord.z * v2;
    return a0 + a1 + a2;
}

void main()
{
    gl_Position = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
    outAttrib = interpolate(inAttrib[0], inAttrib[1], inAttrib[2]);
}
