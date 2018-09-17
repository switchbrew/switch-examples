#pragma once

typedef struct
{
	float x, y, z;
	float nx, ny, nz;
} lennyVertex;

#ifdef __cplusplus
extern "C" {
#endif

extern const lennyVertex lennyVertices[3345];

#ifdef __cplusplus
}
#endif

#define lennyVerticesCount (sizeof(lennyVertices)/sizeof(lennyVertices[0]))
