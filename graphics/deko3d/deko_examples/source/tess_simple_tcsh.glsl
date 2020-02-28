#version 460

layout (vertices = 3) out;

layout (location = 0) in vec4 inAttrib[];

layout (location = 0) out vec4 outAttrib[];

void main()
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 5.0; // i.e. 2 concentric triangles with the center being a triangle
        gl_TessLevelOuter[0] = 2.0;
        gl_TessLevelOuter[1] = 3.0;
        gl_TessLevelOuter[2] = 5.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outAttrib[gl_InvocationID] = inAttrib[gl_InvocationID];
}
