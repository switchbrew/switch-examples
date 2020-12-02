/*
** deko3d Example 09: Simple Compute Shader (Geometry Generation)
** This example shows how to use a compute shader to dynamically generate geometry.
** New concepts in this example:
** - Enabling compute support in a queue
** - Configuring and using compute shaders
** - Setting up shader storage buffers (SSBOs)
** - Dispatching compute jobs
** - Using a primitive barrier to ensure ordering of items
** - Drawing geometry generated dynamically by the GPU itself
*/

// Sample Framework headers
#include "SampleFramework/CApplication.h"
#include "SampleFramework/CMemPool.h"
#include "SampleFramework/CShader.h"
#include "SampleFramework/CCmdMemRing.h"

// C++ standard library headers
#include <array>
#include <optional>

// GLM headers
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // Enforces GLSL std140/std430 alignment rules for glm types
#define GLM_FORCE_INTRINSICS               // Enables usage of SIMD CPU instructions (requiring the above as well)
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    struct Vertex
    {
        float position[4];
        float color[4];
    };

    constexpr std::array VertexAttribState =
    {
        DkVtxAttribState{ 0, 0, offsetof(Vertex, position), DkVtxAttribSize_4x32, DkVtxAttribType_Float, 0 },
        DkVtxAttribState{ 0, 0, offsetof(Vertex, color),    DkVtxAttribSize_4x32, DkVtxAttribType_Float, 0 },
    };

    constexpr std::array VertexBufferState =
    {
        DkVtxBufferState{ sizeof(Vertex), 0 },
    };

    struct GeneratorParams
    {
        glm::vec4 colorA;
        glm::vec4 colorB;
        float offset;
        float scale;
        float padding[2];
    };

    inline float fractf(float x)
    {
        return x - floorf(x);
    }
}

class CExample09 final : public CApplication
{
    static constexpr unsigned NumFramebuffers = 2;
    static constexpr uint32_t FramebufferWidth = 1280;
    static constexpr uint32_t FramebufferHeight = 720;
    static constexpr unsigned StaticCmdSize = 0x10000;
    static constexpr unsigned DynamicCmdSize = 0x10000;
    static constexpr unsigned NumVertices = 256;

    PadState pad;

    dk::UniqueDevice device;
    dk::UniqueQueue queue;

    std::optional<CMemPool> pool_images;
    std::optional<CMemPool> pool_code;
    std::optional<CMemPool> pool_data;

    dk::UniqueCmdBuf cmdbuf;
    dk::UniqueCmdBuf dyncmd;
    CCmdMemRing<NumFramebuffers> dynmem;

    GeneratorParams params;
    CMemPool::Handle paramsUniformBuffer;

    CShader computeShader;
    CShader vertexShader;
    CShader fragmentShader;

    CMemPool::Handle vertexBuffer;

    CMemPool::Handle framebuffers_mem[NumFramebuffers];
    dk::Image framebuffers[NumFramebuffers];
    DkCmdList framebuffer_cmdlists[NumFramebuffers];
    dk::UniqueSwapchain swapchain;

    DkCmdList compute_cmdlist, render_cmdlist;

public:
    CExample09()
    {
        // Create the deko3d device
        device = dk::DeviceMaker{}.create();

        // Create the main queue
        queue = dk::QueueMaker{device}.setFlags(DkQueueFlags_Graphics | DkQueueFlags_Compute).create();

        // Create the memory pools
        pool_images.emplace(device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 16*1024*1024);
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
        computeShader.load(*pool_code, "romfs:/shaders/sinewave.dksh");
        vertexShader.load(*pool_code, "romfs:/shaders/basic_vsh.dksh");
        fragmentShader.load(*pool_code, "romfs:/shaders/color_fsh.dksh");

        // Create the uniform buffer
        paramsUniformBuffer = pool_data->allocate(sizeof(params), DK_UNIFORM_BUF_ALIGNMENT);

        // Initialize the params
        params.colorA = glm::vec4 { 1.0f, 0.0f, 1.0f, 1.0f };
        params.colorB = glm::vec4 { 0.0f, 1.0f, 0.0f, 1.0f };
        params.offset = 0.0f;
        params.scale  = 1.0f;

        // Allocate memory for the vertex buffer
        vertexBuffer = pool_data->allocate(sizeof(Vertex)*NumVertices, alignof(Vertex));

        // Create the framebuffer resources
        createFramebufferResources();

        // Initialize gamepad
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
    }

    ~CExample09()
    {
        // Destroy the framebuffer resources
        destroyFramebufferResources();

        // Destroy the vertex buffer (not strictly needed in this case)
        vertexBuffer.destroy();

        // Destroy the uniform buffer (not strictly needed in this case)
        paramsUniformBuffer.destroy();
    }

    void createFramebufferResources()
    {
        // Create layout for the framebuffers
        dk::ImageLayout layout_framebuffer;
        dk::ImageLayoutMaker{device}
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_RGBA8_Unorm)
            .setDimensions(FramebufferWidth, FramebufferHeight)
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

            // Generate a command list that binds it
            dk::ImageView colorTarget{ framebuffers[i] };
            cmdbuf.bindRenderTargets(&colorTarget);
            framebuffer_cmdlists[i] = cmdbuf.finishList();

            // Fill in the array for use later by the swapchain creation code
            fb_array[i] = &framebuffers[i];
        }

        // Create the swapchain using the framebuffers
        swapchain = dk::SwapchainMaker{device, nwindowGetDefault(), fb_array}.create();

        // Generate the main rendering cmdlist
        recordStaticCommands();
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
    }

    void recordStaticCommands()
    {
        // Bind state required for running the compute job
        cmdbuf.bindShaders(DkStageFlag_Compute, { computeShader });
        cmdbuf.bindUniformBuffer(DkStage_Compute, 0, paramsUniformBuffer.getGpuAddr(), paramsUniformBuffer.getSize());
        cmdbuf.bindStorageBuffer(DkStage_Compute, 0, vertexBuffer.getGpuAddr(), vertexBuffer.getSize());

        // Run the compute job
        cmdbuf.dispatchCompute(NumVertices/32, 1, 1);

        // Place a barrier
        cmdbuf.barrier(DkBarrier_Primitives, 0);

        // Finish off this command list
        compute_cmdlist = cmdbuf.finishList();

        // Initialize state structs with deko3d defaults
        dk::RasterizerState rasterizerState;
        dk::ColorState colorState;
        dk::ColorWriteState colorWriteState;
        dk::BlendState blendState;

        // Configure rasterizer state: enable polygon smoothing
        rasterizerState.setPolygonSmoothEnable(true);

        // Configure color state: enable blending (needed for polygon smoothing since it generates alpha values)
        colorState.setBlendEnable(0, true);

        // Configure viewport and scissor
        cmdbuf.setViewports(0, { { 0.0f, 0.0f, FramebufferWidth, FramebufferHeight, 0.0f, 1.0f } });
        cmdbuf.setScissors(0, { { 0, 0, FramebufferWidth, FramebufferHeight } });

        // Clear the color buffer
        cmdbuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);

        // Bind state required for drawing the triangle
        cmdbuf.bindShaders(DkStageFlag_GraphicsMask, { vertexShader, fragmentShader });
        cmdbuf.bindRasterizerState(rasterizerState);
        cmdbuf.bindColorState(colorState);
        cmdbuf.bindColorWriteState(colorWriteState);
        cmdbuf.bindBlendStates(0, blendState);
        cmdbuf.bindVtxBuffer(0, vertexBuffer.getGpuAddr(), vertexBuffer.getSize());
        cmdbuf.bindVtxAttribState(VertexAttribState);
        cmdbuf.bindVtxBufferState(VertexBufferState);
        cmdbuf.setLineWidth(16.0f);

        // Draw the line
        cmdbuf.draw(DkPrimitive_LineStrip, NumVertices, 1, 0, 0);

        // Finish off this command list
        render_cmdlist = cmdbuf.finishList();
    }

    void render()
    {
        // Begin generating the dynamic command list, for commands that need to be sent only this frame specifically
        dynmem.begin(dyncmd);

        // Update the uniform buffer with the new state (this data gets inlined in the command list)
        dyncmd.pushConstants(
            paramsUniformBuffer.getGpuAddr(), paramsUniformBuffer.getSize(),
            0, sizeof(params), &params);

        // Finish off the dynamic command list (which also submits it to the queue)
        queue.submitCommands(dynmem.end(dyncmd));

        // Run the compute command list
        queue.submitCommands(compute_cmdlist);

        // Acquire a framebuffer from the swapchain (and wait for it to be available)
        int slot = queue.acquireImage(swapchain);

        // Run the command list that attaches said framebuffer to the queue
        queue.submitCommands(framebuffer_cmdlists[slot]);

        // Run the main rendering command list
        queue.submitCommands(render_cmdlist);

        // Now that we are done rendering, present it to the screen
        queue.presentImage(swapchain, slot);
    }

    bool onFrame(u64 ns) override
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus)
            return false;

        float time = ns / 1000000000.0; // double precision division; followed by implicit cast to single precision
        float tau = glm::two_pi<float>();

        params.offset = fractf(time/4.0f);

        float xx = fractf(time * 135.0f / 60.0f / 2.0f);
        params.scale = cosf(xx*tau);
        params.colorA.g = powf(fabsf(params.scale), 4.0f);
        params.colorB.g = 1.0f - params.colorA.g;

        render();
        return true;
    }
};

void Example09(void)
{
    CExample09 app;
    app.run();
}
