/*
** deko3d Example 07: Mesh Loading and Lighting (sRGB)
** This example shows how to load a mesh, and render it using per-fragment lighting.
** New concepts in this example:
** - Loading geometry data (mesh) from the filesystem
** - Configuring and using index buffers
** - Using sRGB framebuffers
** - Using multiple uniform buffers on different stages
** - Basic Blinn-Phong lighting with Reinhard tone mapping
*/

// Sample Framework headers
#include "SampleFramework/CApplication.h"
#include "SampleFramework/CMemPool.h"
#include "SampleFramework/CShader.h"
#include "SampleFramework/CCmdMemRing.h"
#include "SampleFramework/FileLoader.h"

// C++ standard library headers
#include <array>
#include <optional>

// GLM headers
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // Enforces GLSL std140/std430 alignment rules for glm types
#define GLM_FORCE_INTRINSICS               // Enables usage of SIMD CPU instructions (requiring the above as well)
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    struct Vertex
    {
        float position[3];
        float normal[3];
    };

    constexpr std::array VertexAttribState =
    {
        DkVtxAttribState{ 0, 0, offsetof(Vertex, position), DkVtxAttribSize_3x32, DkVtxAttribType_Float, 0 },
        DkVtxAttribState{ 0, 0, offsetof(Vertex, normal),   DkVtxAttribSize_3x32, DkVtxAttribType_Float, 0 },
    };

    constexpr std::array VertexBufferState =
    {
        DkVtxBufferState{ sizeof(Vertex), 0 },
    };

    struct Transformation
    {
        glm::mat4 mdlvMtx;
        glm::mat4 projMtx;
    };

    struct Lighting
    {
        glm::vec4 lightPos; // if w=0 this is lightDir
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec4 specular; // w is shininess
    };

    inline float fractf(float x)
    {
        return x - floorf(x);
    }
}

class CExample07 final : public CApplication
{
    static constexpr unsigned NumFramebuffers = 2;
    static constexpr unsigned StaticCmdSize = 0x10000;
    static constexpr unsigned DynamicCmdSize = 0x10000;
    static constexpr DkMsMode MultisampleMode = DkMsMode_4x;

    PadState pad;

    dk::UniqueDevice device;
    dk::UniqueQueue queue;

    std::optional<CMemPool> pool_images;
    std::optional<CMemPool> pool_code;
    std::optional<CMemPool> pool_data;

    dk::UniqueCmdBuf cmdbuf;
    dk::UniqueCmdBuf dyncmd;
    CCmdMemRing<NumFramebuffers> dynmem;

    CShader vertexShader;
    CShader fragmentShader;

    Transformation transformState;
    CMemPool::Handle transformUniformBuffer;

    Lighting lightingState;
    CMemPool::Handle lightingUniformBuffer;

    CMemPool::Handle vertexBuffer;
    CMemPool::Handle indexBuffer;

    uint32_t framebufferWidth;
    uint32_t framebufferHeight;

    CMemPool::Handle colorBuffer_mem;
    CMemPool::Handle depthBuffer_mem;
    CMemPool::Handle framebuffers_mem[NumFramebuffers];

    dk::Image colorBuffer;
    dk::Image depthBuffer;
    dk::Image framebuffers[NumFramebuffers];
    DkCmdList framebuffer_cmdlists[NumFramebuffers];
    dk::UniqueSwapchain swapchain;

    DkCmdList render_cmdlist, discard_cmdlist;

public:
    CExample07()
    {
        // Create the deko3d device
        device = dk::DeviceMaker{}.create();

        // Create the main queue
        queue = dk::QueueMaker{device}.setFlags(DkQueueFlags_Graphics).create();

        // Create the memory pools
        pool_images.emplace(device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 64*1024*1024);
        pool_code.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code, 128*1024);
        pool_data.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 1*1024*1024);

        // Create the static command buffer and feed it freshly allocated memory
        cmdbuf = dk::CmdBufMaker{device}.create();
        CMemPool::Handle cmdmem = pool_data->allocate(StaticCmdSize);
        cmdbuf.addMemory(cmdmem.getMemBlock(), cmdmem.getOffset(), cmdmem.getSize());

        // Create the dynamic command buffer and allocate memory for it
        dyncmd = dk::CmdBufMaker{device}.create();
        dynmem.allocate(*pool_data, DynamicCmdSize);

        // Load the shaders
        vertexShader.load(*pool_code, "romfs:/shaders/transform_normal_vsh.dksh");
        fragmentShader.load(*pool_code, "romfs:/shaders/basic_lighting_fsh.dksh");

        // Create the transformation uniform buffer
        transformUniformBuffer = pool_data->allocate(sizeof(transformState), DK_UNIFORM_BUF_ALIGNMENT);

        // Create the lighting uniform buffer
        lightingUniformBuffer = pool_data->allocate(sizeof(lightingState), DK_UNIFORM_BUF_ALIGNMENT);

        // Initialize the lighting state
        lightingState.lightPos = glm::vec4{0.0f, 4.0f, 1.0f, 1.0f};
        lightingState.ambient = glm::vec3{0.046227f,0.028832f,0.003302f};
        lightingState.diffuse = glm::vec3{0.564963f,0.367818f,0.051293f};
        lightingState.specular = glm::vec4{24.0f*glm::vec3{0.394737f,0.308916f,0.134004f}, 64.0f};

        // Load the teapot mesh
        vertexBuffer = LoadFile(*pool_data, "romfs:/teapot-vtx.bin", alignof(Vertex));
        indexBuffer = LoadFile(*pool_data, "romfs:/teapot-idx.bin", alignof(u16));

        // Initialize gamepad
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
    }

    ~CExample07()
    {
        // Destroy the framebuffer resources
        destroyFramebufferResources();

        // Destroy the index buffer (not strictly needed in this case)
        indexBuffer.destroy();

        // Destroy the vertex buffer (not strictly needed in this case)
        vertexBuffer.destroy();

        // Destroy the uniform buffer (not strictly needed in this case)
        transformUniformBuffer.destroy();
    }

    void createFramebufferResources()
    {
        // Create layout for the (multisampled) color buffer
        dk::ImageLayout layout_colorbuffer;
        dk::ImageLayoutMaker{device}
            .setType(DkImageType_2DMS)
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_Usage2DEngine | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_RGBA8_Unorm_sRGB)
            .setMsMode(MultisampleMode)
            .setDimensions(framebufferWidth, framebufferHeight)
            .initialize(layout_colorbuffer);

        // Create layout for the (also multisampled) depth buffer
        dk::ImageLayout layout_depthbuffer;
        dk::ImageLayoutMaker{device}
            .setType(DkImageType_2DMS)
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_Z24S8)
            .setMsMode(MultisampleMode)
            .setDimensions(framebufferWidth, framebufferHeight)
            .initialize(layout_depthbuffer);

        // Create the color buffer
        colorBuffer_mem = pool_images->allocate(layout_colorbuffer.getSize(), layout_colorbuffer.getAlignment());
        colorBuffer.initialize(layout_colorbuffer, colorBuffer_mem.getMemBlock(), colorBuffer_mem.getOffset());

        // Create the depth buffer
        depthBuffer_mem = pool_images->allocate(layout_depthbuffer.getSize(), layout_depthbuffer.getAlignment());
        depthBuffer.initialize(layout_depthbuffer, depthBuffer_mem.getMemBlock(), depthBuffer_mem.getOffset());

        // Create layout for the framebuffers
        dk::ImageLayout layout_framebuffer;
        dk::ImageLayoutMaker{device}
            .setFlags(DkImageFlags_Usage2DEngine | DkImageFlags_UsagePresent)
            .setFormat(DkImageFormat_RGBA8_Unorm_sRGB)
            .setDimensions(framebufferWidth, framebufferHeight)
            .initialize(layout_framebuffer);

        // Create the framebuffers
        std::array<DkImage const*, NumFramebuffers> fb_array;
        uint64_t fb_size  = layout_framebuffer.getSize();
        uint32_t fb_align = layout_framebuffer.getAlignment();
        for (unsigned i = 0; i < NumFramebuffers; i ++)
        {
            // Allocate a framebuffer
            framebuffers_mem[i] = pool_images->allocate(fb_size, fb_align);
            framebuffers[i].initialize(layout_framebuffer, framebuffers_mem[i].getMemBlock(), framebuffers_mem[i].getOffset());

            // Generate a command list that resolves the color buffer into the framebuffer
            dk::ImageView colorView { colorBuffer }, framebufferView { framebuffers[i] };
            cmdbuf.resolveImage(colorView, framebufferView);
            framebuffer_cmdlists[i] = cmdbuf.finishList();

            // Fill in the array for use later by the swapchain creation code
            fb_array[i] = &framebuffers[i];
        }

        // Create the swapchain using the framebuffers
        swapchain = dk::SwapchainMaker{device, nwindowGetDefault(), fb_array}.create();

        // Generate the main command lists
        recordStaticCommands();

        // Initialize the projection matrix
        transformState.projMtx = glm::perspectiveRH_ZO(
            glm::radians(40.0f),
            float(framebufferWidth)/float(framebufferHeight),
            0.01f, 1000.0f);
    }

    void destroyFramebufferResources()
    {
        // Return early if we have nothing to destroy
        if (!swapchain) return;

        // Make sure the queue is idle before destroying anything
        queue.waitIdle();

        // Clear the static cmdbuf, destroying the static cmdlists in the process
        cmdbuf.clear();

        // Destroy the swapchain
        swapchain.destroy();

        // Destroy the framebuffers
        for (unsigned i = 0; i < NumFramebuffers; i ++)
            framebuffers_mem[i].destroy();

        // Destroy the depth buffer
        depthBuffer_mem.destroy();

        // Destroy the color buffer
        colorBuffer_mem.destroy();
    }

    void recordStaticCommands()
    {
        // Initialize state structs with deko3d defaults
        dk::RasterizerState rasterizerState;
        dk::MultisampleState multisampleState;
        dk::ColorState colorState;
        dk::ColorWriteState colorWriteState;
        dk::DepthStencilState depthStencilState;

        // Configure multisample state
        multisampleState.setMode(MultisampleMode);
        multisampleState.setLocations();

        // Bind color buffer and depth buffer
        dk::ImageView colorTarget { colorBuffer }, depthTarget { depthBuffer };
        cmdbuf.bindRenderTargets(&colorTarget, &depthTarget);

        // Configure viewport and scissor
        cmdbuf.setViewports(0, { { 0.0f, 0.0f, (float)framebufferWidth, (float)framebufferHeight, 0.0f, 1.0f } });
        cmdbuf.setScissors(0, { { 0, 0, framebufferWidth, framebufferHeight } });

        // Clear the color and depth buffers
        cmdbuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);
        cmdbuf.clearDepthStencil(true, 1.0f, 0xFF, 0);

        // Bind state required for drawing the mesh
        cmdbuf.bindShaders(DkStageFlag_GraphicsMask, { vertexShader, fragmentShader });
        cmdbuf.bindUniformBuffer(DkStage_Vertex, 0, transformUniformBuffer.getGpuAddr(), transformUniformBuffer.getSize());
        cmdbuf.bindUniformBuffer(DkStage_Fragment, 0, lightingUniformBuffer.getGpuAddr(), lightingUniformBuffer.getSize());
        cmdbuf.bindRasterizerState(rasterizerState);
        cmdbuf.bindMultisampleState(multisampleState);
        cmdbuf.bindColorState(colorState);
        cmdbuf.bindColorWriteState(colorWriteState);
        cmdbuf.bindDepthStencilState(depthStencilState);
        cmdbuf.bindVtxBuffer(0, vertexBuffer.getGpuAddr(), vertexBuffer.getSize());
        cmdbuf.bindVtxAttribState(VertexAttribState);
        cmdbuf.bindVtxBufferState(VertexBufferState);
        cmdbuf.bindIdxBuffer(DkIdxFormat_Uint16, indexBuffer.getGpuAddr());

        // Draw the mesh
        cmdbuf.drawIndexed(DkPrimitive_Triangles, indexBuffer.getSize() / sizeof(u16), 1, 0, 0, 0);

        // Finish off this command list
        render_cmdlist = cmdbuf.finishList();

        // Discard the color and depth buffers since we don't need them anymore
        cmdbuf.bindRenderTargets(&colorTarget, &depthTarget);
        cmdbuf.discardColor(0);
        cmdbuf.discardDepthStencil();

        // Finish off this command list
        discard_cmdlist = cmdbuf.finishList();
    }

    void render()
    {
        // Begin generating the dynamic command list, for commands that need to be sent only this frame specifically
        dynmem.begin(dyncmd);

        // Update the transformation uniform buffer with the new state (this data gets inlined in the command list)
        dyncmd.pushConstants(
            transformUniformBuffer.getGpuAddr(), transformUniformBuffer.getSize(),
            0, sizeof(transformState), &transformState);

        // Update the lighting uniform buffer with the new state
        dyncmd.pushConstants(
            lightingUniformBuffer.getGpuAddr(), lightingUniformBuffer.getSize(),
            0, sizeof(lightingState), &lightingState);

        // Finish off the dynamic command list (which also submits it to the queue)
        queue.submitCommands(dynmem.end(dyncmd));

        // Run the main rendering command list
        queue.submitCommands(render_cmdlist);

        // Acquire a framebuffer from the swapchain
        int slot = queue.acquireImage(swapchain);

        // Submit the command list that resolves the color buffer to the framebuffer
        queue.submitCommands(framebuffer_cmdlists[slot]);

        // Submit the command list used for discarding the color and depth buffers
        queue.submitCommands(discard_cmdlist);

        // Now that we are done rendering, present it to the screen (this also flushes the queue)
        queue.presentImage(swapchain, slot);
    }

    void onOperationMode(AppletOperationMode mode) override
    {
        // Destroy the framebuffer resources
        destroyFramebufferResources();

        // Choose framebuffer size
        chooseFramebufferSize(framebufferWidth, framebufferHeight, mode);

        // Recreate the framebuffers and its associated resources
        createFramebufferResources();
    }

    bool onFrame(u64 ns) override
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus)
            return false;

        float time = ns / 1000000000.0; // double precision division; followed by implicit cast to single precision
        float tau = glm::two_pi<float>();

        float period1 = fractf(time/8.0f);
        float period2 = fractf(time/4.0f);

        // Generate the model-view matrix for this frame
        // Keep in mind that GLM transformation functions multiply to the right, so essentially we have:
        //   mdlvMtx = Translate1 * RotateX * RotateY * Translate2
        // This means that the Translate2 operation is applied first, then RotateY, and so on.
        transformState.mdlvMtx = glm::mat4{1.0f};
        transformState.mdlvMtx = glm::translate(transformState.mdlvMtx, glm::vec3{0.0f, 0.0f, -3.0f});
        transformState.mdlvMtx = glm::rotate(transformState.mdlvMtx, sinf(period2 * tau) * tau / 8.0f, glm::vec3{1.0f, 0.0f, 0.0f});
        transformState.mdlvMtx = glm::rotate(transformState.mdlvMtx, -period1 * tau, glm::vec3{0.0f, 1.0f, 0.0f});
        transformState.mdlvMtx = glm::translate(transformState.mdlvMtx, glm::vec3{0.0f, -0.5f, 0.0f});

        render();
        return true;
    }
};

void Example07(void)
{
    CExample07 app;
    app.run();
}
