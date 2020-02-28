#version 460

/*
ID | gl_Position.xy
0  | -1.0 +1.0
1  | -1.0 -1.0
2  | +1.0 -1.0
3  | +1.0 +1.0
*/

void main()
{
    if ((gl_VertexID & 2) == 0)
        gl_Position.x = -1.0;
    else
        gl_Position.x = +1.0;

    if (((gl_VertexID+1) & 2) == 0)
        gl_Position.y = +1.0;
    else
        gl_Position.y = -1.0;

    gl_Position.zw = vec2(0.5, 1.0);
}
