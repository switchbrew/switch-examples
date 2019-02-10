#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <vector>

#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)

// GLM headers
#define GLM_FORCE_PURE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

constexpr auto TAU = glm::two_pi<float>();

//-----------------------------------------------------------------------------
// nxlink support
//-----------------------------------------------------------------------------

#ifndef ENABLE_NXLINK
#define TRACE(fmt,...) ((void)0)
#else
#include <unistd.h>
#define TRACE(fmt,...) printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)

static int s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        TRACE("printf output now goes to nxlink server");
    else
        socketExit();
}

static void deinitNxLink()
{
    if (s_nxlinkSock >= 0)
    {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}

extern "C" void userAppInit()
{
    initNxLink();
}

extern "C" void userAppExit()
{
    deinitNxLink();
}

#endif

//-----------------------------------------------------------------------------
// EGL initialization
//-----------------------------------------------------------------------------

static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;

static bool initEgl(NWindow* win)
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
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0)
    {
        TRACE("No config found! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL window surface
    s_surface = eglCreateWindowSurface(s_display, config, win, nullptr);
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

static void deinitEgl()
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

//-----------------------------------------------------------------------------
// Main program
//-----------------------------------------------------------------------------

static void setMesaConfig()
{
    // Uncomment below to disable error checking and save CPU time (useful for production):
    //setenv("MESA_NO_ERROR", "1", 1);

    // Uncomment below to enable Mesa logging:
    //setenv("EGL_LOG_LEVEL", "debug", 1);
    //setenv("MESA_VERBOSE", "all", 1);
    //setenv("NOUVEAU_MESA_DEBUG", "1", 1);

    // Uncomment below to enable shader debugging in Nouveau:
    //setenv("NV50_PROG_OPTIMIZE", "0", 1);
    //setenv("NV50_PROG_DEBUG", "1", 1);
    //setenv("NV50_PROG_CHIPSET", "0x120", 1);
}

static const char* const vertexShaderSource = R"text(
    #version 320 es
    precision mediump float;

    layout (location = 0) in vec3 inPos;
    layout (location = 1) in vec2 inTexCoord;
    layout (location = 2) in vec3 inNormal;

    out vec2 vtxTexCoord;
    out vec4 vtxNormalQuat;
    out vec3 vtxView;

    uniform mat4 mdlvMtx;
    uniform mat4 projMtx;

    void main()
    {
        // Calculate position
        vec4 pos = mdlvMtx * vec4(inPos, 1.0);
        vtxView = -pos.xyz;
        gl_Position = projMtx * pos;

        // Calculate normalquat
        vec3 normal = normalize(mat3(mdlvMtx) * inNormal);
        float z = (1.0 + normal.z) / 2.0;
        vtxNormalQuat = vec4(1.0, 0.0, 0.0, 0.0);
        if (z > 0.0)
        {
            vtxNormalQuat.z = sqrt(z);
            vtxNormalQuat.xy = normal.xy / (2.0 * vtxNormalQuat.z);
        }

        // Calculate texcoord
        vtxTexCoord = inTexCoord;
    }
)text";

static const char* const fragmentShaderSource = R"text(
    #version 320 es
    precision mediump float;

    in vec2 vtxTexCoord;
    in vec4 vtxNormalQuat;
    in vec3 vtxView;

    out vec4 fragColor;

    uniform vec4 lightPos;
    uniform vec3 lightAmbient;
    uniform vec3 lightDiffuse;
    uniform vec4 lightSpecular; // w component is shininess

    uniform sampler2D tex_diffuse;
	uniform sampler2D tex_specular;
	uniform sampler2D tex_ambOcc;
	uniform sampler2D tex_normal;
	uniform sampler2D tex_rough;
	
	mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
	{
		// get edge vectors of the pixel triangle
		vec3 dp1 = dFdx( p );
		vec3 dp2 = dFdy( p );
		vec2 duv1 = dFdx( uv );
		vec2 duv2 = dFdy( uv );
 
		// solve the linear system
		vec3 dp2perp = cross( dp2, N );
		vec3 dp1perp = cross( N, dp1 );
		vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
		vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
		// construct a scale-invariant frame 
		float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
		return mat3( T * invmax, B * invmax, N );
	}
	
	vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
	{
		vec3 map = texture( tex_normal, texcoord ).xyz;
	
		mat3 TBN = cotangent_frame( N, -V, texcoord );
		return normalize( TBN * map );
	}

    void main()
    {
        vec4 texDiffuseColor = texture(tex_diffuse, vtxTexCoord);
		vec4 texSpecularColor = texture(tex_specular, vtxTexCoord);
		vec4 texAmbientColor = texture(tex_ambOcc, vtxTexCoord);
		vec4 texRoughColor = texture(tex_rough, vtxTexCoord);
		
        vec3 normal = normalize(vtxNormalQuat.xyz);
        vec3 lightVec = normalize(lightPos.xyz + vtxView);
        vec3 viewVec = normalize(vtxView);
        vec3 halfVec = normalize(viewVec + lightVec);
		
		normal = perturb_normal(normal, lightVec, vtxTexCoord);
		
        float diffuseFactor = max(dot(lightVec, normal), 0.0);
        float specularFactor = pow(max(dot(normal, halfVec), 0.0), lightSpecular.w);

		vec3 finalAmbient = texAmbientColor.rgb * lightAmbient;
		vec3 finalSpecular = texSpecularColor.r * specularFactor* lightSpecular.xyz;
		vec3 finalDiffuse = texDiffuseColor.rgb * diffuseFactor * lightDiffuse.xyz;
		
        vec3 fragLightColor = finalAmbient + finalDiffuse + finalSpecular;
		
		fragLightColor *= ( vec3(1) - texRoughColor.xyz   );

        fragColor = vec4(fragLightColor, 1.0);
    }
)text";

static GLuint createAndCompileShader(GLenum type, const char* source)
{
    GLint success;
    GLchar msg[512];

    GLuint handle = glCreateShader(type);
    if (!handle)
    {
        TRACE("%u: cannot create shader", type);
        return 0;
    }
    glShaderSource(handle, 1, &source, nullptr);
    glCompileShader(handle);
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(handle, sizeof(msg), nullptr, msg);
        TRACE("%u: %s\n", type, msg);
        glDeleteShader(handle);
        return 0;
    }

    return handle;
}

typedef struct
{
    float position[3];
    float texcoord[2];
    float normal[3];
} Vertex;

static GLuint s_program;
static GLuint s_vao, s_vbo;

static GLint loc_mdlvMtx, loc_projMtx;
static GLint loc_lightPos, loc_ambient, loc_diffuse, loc_specular;


static GLuint s_tex_diffuse, s_tex_specular, s_tex_ambOcc, s_tex_normal, s_tex_rough;
static GLint loc_tex_diffuse, loc_tex_specular, loc_tex_ambOcc, loc_tex_normal, loc_tex_rough;

static u64 s_startTicks;

void readPNG(const char* path, char** data, int* size)
{	
	FILE *fp = fopen ( path , "rb" );

    fseek( fp , 0L , SEEK_END);
    *size = ftell( fp );
    rewind( fp );

    *data = (char*)calloc( 1, *size+1 );
    fread( *data , *size, 1 , fp);

    fclose(fp);
}

Vertex* gunList;
int gunNumVerts;

void readOBJ(const char* path)
{
    FILE* f = fopen(path, "r");

	float x[3];
	unsigned short y[9];
	
	std::vector<float>verts;
	std::vector<float>uvs;
	std::vector<float>norms;
	std::vector<unsigned short>faces;
	
    char line[100];
    while (fgets(line, sizeof(line), f))
    {
		if (sscanf(line, "v %f %f %f", &x[0], &x[1], &x[2]) == 3)
		{
			for(int i = 0; i < 3; i++)
				verts.push_back(x[i]);
		}
		
		if (sscanf(line, "vt %f %f", &x[0], &x[1]) == 2)
		{
			for(int i = 0; i < 2; i++)
				uvs.push_back(x[i]);
		}
			
		if (sscanf(line, "vn %f %f %f", &x[0], &x[1], &x[2]) == 3)
		{
			for(int i = 0; i < 3; i++)
				norms.push_back(x[i]);
			
		}

		if (sscanf(line, "f %hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu", &y[0], &y[1], &y[2], &y[3], &y[4], &y[5], &y[6], &y[7], &y[8]) == 9)
		{
			for(int i = 0; i < 9; i++)
				faces.push_back(y[i]-1);
		}
    }
	
	int numFaces = faces.size()/9;
	
	gunNumVerts = numFaces*3;
	gunList = new Vertex[gunNumVerts];
	
	for(int i = 0; i < (int)numFaces; i++)
	{		
		for(int j = 0; j < 3; j++)
		{
			Vertex v;
			
			for(int k = 0; k < 3; k++)
				v.position[k] = verts[   3*faces[9*i+3*j+0]  +k];
			
			
			v.texcoord[0] =   uvs[       2*faces[9*i+3*j+1]  +0];
			v.texcoord[1] = 1-uvs[       2*faces[9*i+3*j+1]  +1];
			
			for(int k = 0; k < 3; k++)
				v.normal[k] =   norms[   3*faces[9*i+3*j+2]  +k];
			
			gunList[3*i + j] = v;	
		}
	}
	
    fclose(f);

}

char* AmbOcc_png;
int AmbOcc_png_size;

char* Diffuse_png;
int Diffuse_png_size;

char* Normal_png;
int Normal_png_size;

char* Rough_png;
int Rough_png_size;

char* Specular_png;
int Specular_png_size;

static void sceneInit()
{
	consoleInit(NULL);
    romfsInit();
    
    readPNG("romfs:/AmbOcc.png", &AmbOcc_png, &AmbOcc_png_size);
	readPNG("romfs:/Diffuse.png", &Diffuse_png, &Diffuse_png_size);
	readPNG("romfs:/Normal.png", &Normal_png, &Normal_png_size);
	readPNG("romfs:/Rough.png", &Rough_png, &Rough_png_size);
	readPNG("romfs:/Specular.png", &Specular_png, &Specular_png_size);
	
	readOBJ("romfs:/gun.3Dobj");
	//readOBJ("romfs:/cube.3Dobj");
	
    GLint vsh = createAndCompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLint fsh = createAndCompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    s_program = glCreateProgram();
    glAttachShader(s_program, vsh);
    glAttachShader(s_program, fsh);
    glLinkProgram(s_program);

    GLint success;
    glGetProgramiv(s_program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        char buf[512];
        glGetProgramInfoLog(s_program, sizeof(buf), nullptr, buf);
        TRACE("Link error: %s", buf);
    }
    glDeleteShader(vsh);
    glDeleteShader(fsh);

    loc_mdlvMtx = glGetUniformLocation(s_program, "mdlvMtx");
    loc_projMtx = glGetUniformLocation(s_program, "projMtx");
    loc_lightPos = glGetUniformLocation(s_program, "lightPos");
    loc_ambient = glGetUniformLocation(s_program, "lightAmbient");
    loc_diffuse = glGetUniformLocation(s_program, "lightDiffuse");
    loc_specular = glGetUniformLocation(s_program, "lightSpecular");
    loc_tex_diffuse = glGetUniformLocation(s_program, "tex_diffuse");
	loc_tex_specular = glGetUniformLocation(s_program, "tex_specular");
	loc_tex_ambOcc = glGetUniformLocation(s_program, "tex_ambOcc");
	loc_tex_normal = glGetUniformLocation(s_program, "tex_normal");
	loc_tex_rough = glGetUniformLocation(s_program, "tex_rough");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(s_vao);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*gunNumVerts, gunList, GL_STATIC_DRAW);
	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    int width, height, nchan;
	stbi_uc* img;
	
    // Textures
    glGenTextures(1, &s_tex_diffuse);
    glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, s_tex_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    img = stbi_load_from_memory((const stbi_uc*)Diffuse_png, Diffuse_png_size, &width, &height, &nchan, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);
	
	// Textures
    glGenTextures(1, &s_tex_specular);
    glActiveTexture(GL_TEXTURE1); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, s_tex_specular);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    img = stbi_load_from_memory((const stbi_uc*)Specular_png, Specular_png_size, &width, &height, &nchan, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);

	// Textures
    glGenTextures(1, &s_tex_ambOcc);
    glActiveTexture(GL_TEXTURE2); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, s_tex_ambOcc);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    img = stbi_load_from_memory((const stbi_uc*)AmbOcc_png, AmbOcc_png_size, &width, &height, &nchan, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);	
	
	// Textures
    glGenTextures(1, &s_tex_normal);
    glActiveTexture(GL_TEXTURE3); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, s_tex_normal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    img = stbi_load_from_memory((const stbi_uc*)Normal_png, Normal_png_size, &width, &height, &nchan, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);

	// Textures
    glGenTextures(1, &s_tex_rough);
    glActiveTexture(GL_TEXTURE4); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, s_tex_rough);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    img = stbi_load_from_memory((const stbi_uc*)Rough_png, Rough_png_size, &width, &height, &nchan, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    stbi_image_free(img);	

    // Uniforms
    glUseProgram(s_program);
    auto projMtx = glm::perspective(40.0f*TAU/360.0f, 1280.0f/720.0f, 0.01f, 1000.0f);
    glUniformMatrix4fv(loc_projMtx, 1, GL_FALSE, glm::value_ptr(projMtx));
    glUniform4f(loc_lightPos, 0.0f, 0.0f, 0.5f, 1.0f);
    glUniform3f(loc_ambient, 0.5f, 0.5f, 0.5f);
	glUniform3f(loc_diffuse, 0.2f, 0.2f, 1.0f);
	glUniform4f(loc_specular, 0.3f, 0.3f, 0.3f, 16.0f);
    glUniform1i(loc_tex_diffuse, 0); // GL_TEXTURE0make 
	glUniform1i(loc_tex_specular, 1); // GL_TEXTURE1
	glUniform1i(loc_tex_ambOcc, 2);
	glUniform1i(loc_tex_normal, 3);
	glUniform1i(loc_tex_rough, 4);
    s_startTicks = armGetSystemTick();
}

static float getTime()
{
    u64 elapsed = armGetSystemTick() - s_startTicks;
    return (elapsed * 625 / 12) / 1000000000.0;
}

static void sceneUpdate()
{
    glm::mat4 mdlvMtx{1.0};
	
	mdlvMtx = glm::translate(mdlvMtx, glm::vec3{0.0f, 0.0f, -2.0f});
	
	//mdlvMtx = glm::rotate(mdlvMtx, getTime() * TAU * 0.00234375f, glm::vec3{1.0f, 0.0f, 0.0f});
    mdlvMtx = glm::rotate(mdlvMtx, getTime() * TAU * 0.00234375f / 2.0f, glm::vec3{0.0f, 1.0f, 0.0f});
    glUniformMatrix4fv(loc_mdlvMtx, 1, GL_FALSE, glm::value_ptr(mdlvMtx));
}

static void sceneRender()
{
    glClearColor(0x68/255.0f, 0xB0/255.0f, 0xD8/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw our textured cube
    glBindVertexArray(s_vao); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    
	glDrawArrays(GL_TRIANGLES, 0, gunNumVerts);
}

static void sceneExit()
{
    glDeleteTextures(1, &s_tex_diffuse);
	glDeleteTextures(1, &s_tex_specular);
	glDeleteTextures(1, &s_tex_ambOcc);
	glDeleteTextures(1, &s_tex_normal);
	glDeleteTextures(1, &s_tex_rough);
    glDeleteBuffers(1, &s_vbo);
    glDeleteVertexArrays(1, &s_vao);
    glDeleteProgram(s_program);
}

int main(int argc, char* argv[])
{
    // Set mesa configuration (useful for debugging)
    setMesaConfig();

    // Initialize EGL on the default window
    if (!initEgl(nwindowGetDefault()))
        return EXIT_FAILURE;

    // Load OpenGL routines using glad
    gladLoadGL();

    // Initialize our scene
    sceneInit();

    // Main graphics loop
    while (appletMainLoop())
    {
        // Get and process input
        hidScanInput();
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_PLUS)
            break;

        // Update our scene
        sceneUpdate();

        // Render stuff!
        sceneRender();
        eglSwapBuffers(s_display, s_surface);
    }

    // Deinitialize our scene
    sceneExit();

    // Deinitialize EGL
    deinitEgl();
    return EXIT_SUCCESS;
}
