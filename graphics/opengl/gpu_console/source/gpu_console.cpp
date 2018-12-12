// Sample GPU/OpenGL powered console renderer implementation.
// Please note that this implementation is incomplete, and only normal/bold colors are implemented.
// Reverse colors, faint colors, underline or strikethrough are not implemented.
// Nevertheless, it should suffice for most purposes.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)

#define TRACE(...) ((void)0)

static const char* const vertexShaderSource = R"text(
#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out gl_PerVertex
{
	vec4 gl_Position;
};

layout (location = 0) in int inAttr;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec3 outUV;

uniform ivec2 dimensions;
uniform vec4 palettes[16] = vec4[](
	vec4(0.0, 0.0, 0.0, 1.0),
	vec4(0.5, 0.0, 0.0, 1.0),
	vec4(0.0, 0.5, 0.0, 1.0),
	vec4(0.5, 0.5, 0.0, 1.0),
	vec4(0.0, 0.0, 0.5, 1.0),
	vec4(0.5, 0.0, 0.5, 1.0),
	vec4(0.0, 0.5, 0.5, 1.0),
	vec4(0.75, 0.75, 0.75, 1.0),
	vec4(0.5, 0.5, 0.5, 1.0),
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0)
);

const vec2 builtin_vertices[] = vec2[](
	vec2(0.0, 0.0),
	vec2(0.0, -1.0),
	vec2(1.0, -1.0),
	vec2(0.0, 0.0),
	vec2(1.0, -1.0),
	vec2(1.0, 0.0)
);

void main()
{
	// Extract data from the attribute
	float tileId = float(inAttr & 0x3FF);
	bool hFlip = ((inAttr >> 10) & 1) != 0;
	bool vFlip = ((inAttr >> 11) & 1) != 0;
	int palId = (inAttr >> 12) & 0xF;

	vec2 vtxData = builtin_vertices[gl_VertexID];

	// Position
	float tileRow = floor(float(gl_InstanceID) / dimensions.x);
	float tileCol = float(gl_InstanceID) - tileRow*dimensions.x;
	vec2 basePos;
	basePos.x = 2.0 * tileCol / dimensions.x - 1.0;
	basePos.y = 2.0 * (1.0 - tileRow / dimensions.y) - 1.0;

	vec2 offsetPos = vec2(2.0) / vec2(dimensions.xy);
	gl_Position.xy = basePos + offsetPos * vtxData;
	gl_Position.zw = vec2(0.0, 1.0);

	// Color
	outColor = palettes[palId];

	// UVs
	if (hFlip)
		vtxData.x = 1.0 - vtxData.x;
	if (vFlip)
		vtxData.y = -1.0 - vtxData.y;
	outUV.xy = vec2(0.0,1.0) + vtxData;
	outUV.z  = tileId;
}
)text";

static const char* const fragmentShaderSource = R"text(
#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec3 inUV;

layout (location = 0) out vec4 outColor;

uniform sampler2DArray tileset;

void main()
{
	float alpha = inColor.a * texture(tileset, inUV).r;
	if (alpha < 0.1) discard;
	outColor.rgb = inColor.rgb;
	outColor.a = alpha;
}
)text";

namespace
{

struct GpuConsole : public ConsoleRenderer
{
	constexpr GpuConsole() :
		ConsoleRenderer{ _init, _deinit, _drawChar, _scrollWindow, _flushAndSwap },
		s_display{}, s_context{}, s_surface{},
		s_tilemapVsh{}, s_tilemapFsh{}, s_tilemapPipeline{},
		s_tilemapVao{}, s_tilemapVbo{}, s_tilemap{},
		s_tilesetTex{}
	{ }

	bool init(PrintConsole* con);
	void deinit(PrintConsole* con);
	void drawChar(PrintConsole* con, int x, int y, int c);
	void scrollWindow(PrintConsole* con);
	void flushAndSwap(PrintConsole* con);

private:
	static GpuConsole* _get(PrintConsole* con)
	{
		return static_cast<GpuConsole*>(con->renderer);
	}

	static bool _init(PrintConsole* con)
	{
		return _get(con)->init(con);
	}

	static void _deinit(PrintConsole* con)
	{
		_get(con)->deinit(con);
	}

	static void _drawChar(PrintConsole* con, int x, int y, int c)
	{
		_get(con)->drawChar(con, x, y, c);
	}

	static void _scrollWindow(PrintConsole* con)
	{
		_get(con)->scrollWindow(con);
	}

	static void _flushAndSwap(PrintConsole* con)
	{
		_get(con)->flushAndSwap(con);
	}

	EGLDisplay s_display;
	EGLContext s_context;
	EGLSurface s_surface;

	bool initEgl();
	void deinitEgl();

	GLuint s_tilemapVsh, s_tilemapFsh;
	GLuint s_tilemapPipeline;
	GLuint s_tilemapVao, s_tilemapVbo;
	uint16_t* s_tilemap;

	GLuint s_tilesetTex;
};

constexpr uint16_t MakeTilemapEntry(unsigned tileId, bool hFlip, bool vFlip, unsigned palId)
{
	uint16_t ent = 0;
	ent |= (tileId & 0x3FF);
	if (hFlip)
		ent |= 1u << 10;
	if (vFlip)
		ent |= 1u << 11;
	ent |= (palId & 0xF) << 12;
	return ent;
}

GLuint loadShaderProgram(GLenum type, const char* source)
{
	GLint success;
	GLchar msg[512];

	GLuint handle = glCreateShaderProgramv(type, 1, &source);
	glGetProgramiv(handle, GL_LINK_STATUS, &success);
	if (success == GL_FALSE)
	{
		glGetProgramInfoLog(handle, sizeof(msg), nullptr, msg);
		TRACE("Shader error: %s", msg);
		glDeleteProgram(handle);
		handle = 0;
	}

	return handle;
}

}

bool GpuConsole::init(PrintConsole* con)
{
	// Initialize EGL and load GL routines
	if (!initEgl())
		return false;
	gladLoadGL();

	// Load our shaders
	s_tilemapVsh = loadShaderProgram(GL_VERTEX_SHADER, vertexShaderSource);
	s_tilemapFsh = loadShaderProgram(GL_FRAGMENT_SHADER, fragmentShaderSource);

	// Configure tilemap dimensions
	glProgramUniform2i(s_tilemapVsh, glGetUniformLocation(s_tilemapVsh, "dimensions"),
		con->consoleWidth, con->consoleHeight
	);

	// Create a program pipeline and attach the programs to their respective stages
	glGenProgramPipelines(1, &s_tilemapPipeline);
	glUseProgramStages(s_tilemapPipeline, GL_VERTEX_SHADER_BIT,   s_tilemapVsh);
	glUseProgramStages(s_tilemapPipeline, GL_FRAGMENT_SHADER_BIT, s_tilemapFsh);

	// Create a VAO and a VBO for the tilemap
	glGenVertexArrays(1, &s_tilemapVao);
	glBindVertexArray(s_tilemapVao);

	// Allocate the tilemap data
	glGenBuffers(1, &s_tilemapVbo);
	glBindBuffer(GL_ARRAY_BUFFER, s_tilemapVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uint16_t)*con->consoleWidth*con->consoleHeight, nullptr, GL_DYNAMIC_DRAW);

	// Configure the only vertex attribute (which is per-instance)
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, sizeof(uint16_t), (void*)0);
	glVertexAttribDivisor(0, 1);
	glEnableVertexAttribArray(0);

	// We're done with the VBO/VAO, unbind them
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Allocate the tilemap and clear it
	s_tilemap = new uint16_t[con->consoleWidth*con->consoleHeight];
	memset(s_tilemap, 0, sizeof(uint16_t)*con->consoleWidth*con->consoleHeight);

	// Unpack 1bpp tileset into a texture image OpenGL can load
	uint8_t* tileset = new uint8_t[con->font.numChars*con->font.tileWidth*con->font.tileHeight];
	unsigned bytesPerRow = (con->font.tileWidth+7)/8;
	for (unsigned i = 0; i < con->font.numChars; i ++)
	{
		const uint8_t* tile = (const uint8_t*)con->font.gfx + con->font.tileHeight*bytesPerRow*i;
		uint8_t* data = &tileset[con->font.tileWidth*con->font.tileHeight*i];
		for (unsigned j = 0; j < con->font.tileHeight; j ++)
		{
			//const uint8_t* row = &tile[bytesPerRow*(con->font.tileHeight-1-j)];
			const uint8_t* row = &tile[bytesPerRow*(con->font.tileHeight-j)];
			uint8_t c = 0;
			for (unsigned k = 0; k < con->font.tileWidth; k ++)
			{
				if (!(k & 7))
					c = *--row;
				*data++ = (c & 0x80) ? 0xFF : 0x00;
				c <<= 1;
			}
		}
	}

	// Create tileset texture from the unpacked tileset image
	glGenTextures(1, &s_tilesetTex);
	glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
	glBindTexture(GL_TEXTURE_2D_ARRAY, s_tilesetTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // can also use GL_LINEAR here
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, con->font.tileWidth, con->font.tileHeight, con->font.numChars, 0, GL_RED, GL_UNSIGNED_BYTE, tileset);
	delete[] tileset;

	// Bind the texture unit to the fragment shader
	glProgramUniform1i(s_tilemapFsh, glGetUniformLocation(s_tilemapFsh, "tileset"), 0); // texunit 0

	// Other miscellaneous init
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Configure viewport
	glViewport(0, 0, 1280, 720);

	return true;
}

void GpuConsole::deinit(PrintConsole* con)
{
	glDeleteTextures(1, &s_tilesetTex);
	glDeleteBuffers(1, &s_tilemapVbo);
	glDeleteVertexArrays(1, &s_tilemapVao);
	glDeleteProgramPipelines(1, &s_tilemapPipeline);
	glDeleteProgram(s_tilemapFsh);
	glDeleteProgram(s_tilemapVsh);
	delete[] s_tilemap;
	deinitEgl();
}

void GpuConsole::drawChar(PrintConsole* con, int x, int y, int c)
{
	int writingColor = con->fg;
	int screenColor = con->bg;

	if (con->flags & CONSOLE_COLOR_BOLD) {
		writingColor += 8;
	} else if (con->flags & CONSOLE_COLOR_FAINT) {
		// Not supported yet
		//writingColor += 16;
	}

	if (con->flags & CONSOLE_COLOR_REVERSE) {
		int tmp = writingColor;
		writingColor = screenColor;
		screenColor = tmp;
	}

	s_tilemap[y*con->consoleWidth+x] = MakeTilemapEntry(c, false, false, writingColor);
}

void GpuConsole::scrollWindow(PrintConsole* con)
{
	for (int y = 0; y < con->windowHeight-1; y ++)
		memcpy(
			&s_tilemap[(con->windowY+y+0)*con->consoleWidth + con->windowX],
			&s_tilemap[(con->windowY+y+1)*con->consoleWidth + con->windowX],
			sizeof(uint16_t)*con->windowWidth);
}

void GpuConsole::flushAndSwap(PrintConsole* con)
{
	// Clear the framebuffer
	glClearColor(0x10/255.0f, 0x10/255.0f, 0x10/255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Update tilemap
	glBindBuffer(GL_ARRAY_BUFFER, s_tilemapVbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(uint16_t)*con->consoleWidth*con->consoleHeight, s_tilemap);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Draw the tilemap
	glBindProgramPipeline(s_tilemapPipeline);
	glBindVertexArray(s_tilemapVao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, con->consoleWidth*con->consoleHeight);
	glBindVertexArray(0);

	// Swap buffers
	eglSwapBuffers(s_display, s_surface);
}

bool GpuConsole::initEgl()
{
	// Connect to the EGL default display
	s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (!s_display)
	{
		TRACE("Could not connect to display! error: %d", eglGetError());
		goto _fail0;
	}

	// Initialize the EGL display connection
	eglInitialize(s_display, nullptr, nullptr);

	// Select OpenGL (Core) as the desired graphics API
	if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
	{
		TRACE("Could not set API! error: %d", eglGetError());
		goto _fail1;
	}

	// Get an appropriate EGL framebuffer configuration
	EGLConfig config;
	EGLint numConfigs;
	static const EGLint framebufferAttributeList[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE,     8,
		EGL_GREEN_SIZE,   8,
		EGL_BLUE_SIZE,    8,
		EGL_ALPHA_SIZE,   8,
		EGL_NONE
	};
	eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
	if (numConfigs == 0)
	{
		TRACE("No config found! error: %d", eglGetError());
		goto _fail1;
	}

	// Create an EGL window surface
	s_surface = eglCreateWindowSurface(s_display, config, nwindowGetDefault(), nullptr);
	if (!s_surface)
	{
		TRACE("Surface creation failed! error: %d", eglGetError());
		goto _fail1;
	}

	// Create an EGL rendering context
	static const EGLint contextAttributeList[] =
	{
		EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
		EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
		EGL_CONTEXT_MINOR_VERSION_KHR, 3,
		EGL_NONE
	};
	s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
	if (!s_context)
	{
		TRACE("Context creation failed! error: %d", eglGetError());
		goto _fail2;
	}

	// Connect the context to the surface
	eglMakeCurrent(s_display, s_surface, s_surface, s_context);
	return true;

_fail2:
	eglDestroySurface(s_display, s_surface);
	s_surface = nullptr;
_fail1:
	eglTerminate(s_display);
	s_display = nullptr;
_fail0:
	return false;
}

void GpuConsole::deinitEgl()
{
	if (s_display)
	{
		eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (s_context)
		{
			eglDestroyContext(s_display, s_context);
			s_context = nullptr;
		}
		if (s_surface)
		{
			eglDestroySurface(s_display, s_surface);
			s_surface = nullptr;
		}
		eglTerminate(s_display);
		s_display = nullptr;
	}
}

extern "C" ConsoleRenderer* getDefaultConsoleRenderer(void)
{
	static GpuConsole s_gpuConsole;
	return &s_gpuConsole;
}
