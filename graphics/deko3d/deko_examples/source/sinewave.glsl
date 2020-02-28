#version 460

layout (local_size_x = 32) in;

struct Vertex
{
	vec4 position;
	vec4 color;
};

layout (std140, binding = 0) uniform Params
{
    vec4 colorA;
	vec4 colorB;
	float offset;
	float scale;
} u;

layout (std430, binding = 0) buffer Output
{
	Vertex vertices[];
} o;

const float TAU = 6.2831853071795;

void calcVertex(out Vertex vtx, float x)
{
	vtx.position = vec4(x * 2.0 - 1.0, u.scale * sin((u.offset + x)*TAU), 0.5, 1.0);
	vtx.color = mix(u.colorA, u.colorB, x);
}

void main()
{
	uint id = gl_GlobalInvocationID.x;
	uint maxid = gl_WorkGroupSize.x * gl_NumWorkGroups.x - 1;
	float x = float(id) / float(maxid);
	calcVertex(o.vertices[id], x);
}
