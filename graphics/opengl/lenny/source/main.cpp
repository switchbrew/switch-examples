#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

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

// ( ͡° ͜ʖ ͡°) mesh data
#include "lenny.h"

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
    #version 330 core

    layout (location = 0) in vec3 inPos;
    layout (location = 1) in vec3 inNormal;

    out vec4 vtxColor;
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

        // Calculate color
        vtxColor = vec4(1.0);
    }
)text";

static const char* const fragmentShaderSource = R"text(
    #version 330 core

    in vec4 vtxColor;
    in vec4 vtxNormalQuat;
    in vec3 vtxView;

    out vec4 fragColor;

    uniform vec4 lightPos;
    uniform vec3 ambient;
    uniform vec3 diffuse;
    uniform vec4 specular; // w component is shininess

    // Rotate the vector v by the quaternion q
    vec3 quatrotate(vec4 q, vec3 v)
    {
        return v + 2.0*cross(q.xyz, cross(q.xyz, v) + q.w*v);
    }

    void main()
    {
        // Extract normal from quaternion
        vec4 normquat = normalize(vtxNormalQuat);
        vec3 normal = quatrotate(normquat, vec3(0.0, 0.0, 1.0));

        vec3 lightVec;
        if (lightPos.w != 0.0)
            lightVec = normalize(lightPos.xyz + vtxView);
        else
            lightVec = normalize(lightPos.xyz);

        vec3 viewVec = normalize(vtxView);
        vec3 halfVec = normalize(viewVec + lightVec);
        float diffuseFactor = max(dot(lightVec, normal), 0.0);
        float specularFactor = pow(max(dot(normal, halfVec), 0.0), specular.w);

        vec3 fragLightColor = ambient + diffuseFactor*diffuse + specularFactor*specular.xyz;

        fragColor = vec4(min(fragLightColor, 1.0), 1.0);
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

static GLuint s_program;
static GLuint s_vao, s_vbo;

static GLint loc_mdlvMtx, loc_projMtx;
static GLint loc_lightPos, loc_ambient, loc_diffuse, loc_specular;

static u64 s_startTicks;

static void sceneInit()
{
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
    loc_ambient = glGetUniformLocation(s_program, "ambient");
    loc_diffuse = glGetUniformLocation(s_program, "diffuse");
    loc_specular = glGetUniformLocation(s_program, "specular");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(s_vao);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lennyVertices), lennyVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(lennyVertex), (void*)offsetof(lennyVertex, x));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(lennyVertex), (void*)offsetof(lennyVertex, nx));
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // Uniforms
    glUseProgram(s_program);
    auto projMtx = glm::perspective(40.0f*TAU/360.0f, 1280.0f/720.0f, 0.01f, 1000.0f);
    glUniformMatrix4fv(loc_projMtx, 1, GL_FALSE, glm::value_ptr(projMtx));
    glUniform4f(loc_lightPos, 0.0f, 0.0f, -0.5f, 1.0f);
    glUniform3f(loc_ambient, 0.1f, 0.1f, 0.1f);
    glUniform3f(loc_diffuse, 0.4f, 0.4f, 0.4f);
    glUniform4f(loc_specular, 0.5f, 0.5f, 0.5f, 20.0f);
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
    mdlvMtx = glm::translate(mdlvMtx, glm::vec3{0.0f, 0.0f, -3.0f});
    mdlvMtx = glm::rotate(mdlvMtx, getTime() * TAU * 0.234375f, glm::vec3{0.0f, 1.0f, 0.0f});
    mdlvMtx = glm::scale(mdlvMtx, glm::vec3{2.0f});
    glUniformMatrix4fv(loc_mdlvMtx, 1, GL_FALSE, glm::value_ptr(mdlvMtx));
}

static void sceneRender()
{
    glClearColor(0x68/255.0f, 0xB0/255.0f, 0xD8/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw our ( ͡° ͜ʖ ͡°)
    glBindVertexArray(s_vao); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, lennyVerticesCount);
}

static void sceneExit()
{
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

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    // Main graphics loop
    while (appletMainLoop())
    {
        // Get and process input
        padUpdate(&pad);
        u32 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus)
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
