/*
** deko3d Example 01: Simple Setup
** This example shows how to setup deko3d for rendering scenes with the GPU.
** New concepts in this example:
** - Creating devices and queues
** - Basic memory management
** - Setting up framebuffers and swapchains
** - Recording a static command list with rendering commands
** - Acquiring and presenting images with the queue and swapchain
*/

// Sample Framework headers
#include "SampleFramework/CApplication.h"
#include "SampleFramework/CMemPool.h"

// C++ standard library headers
#include <array>
#include <optional>

class CExample01 final : public CApplication
{
    static constexpr unsigned NumFramebuffers = 2;
    static constexpr uint32_t FramebufferWidth = 1280;
    static constexpr uint32_t FramebufferHeight = 720;
    static constexpr unsigned StaticCmdSize = 0x1000;

    PadState pad;

    dk::UniqueDevice device;
    dk::UniqueQueue queue;

    std::optional<CMemPool> pool_images;
    std::optional<CMemPool> pool_data;

    dk::UniqueCmdBuf cmdbuf;

    CMemPool::Handle framebuffers_mem[NumFramebuffers];
    dk::Image framebuffers[NumFramebuffers];
    DkCmdList framebuffer_cmdlists[NumFramebuffers];
    dk::UniqueSwapchain swapchain;

    DkCmdList render_cmdlist;

public:
    CExample01()
    {
        // Create the deko3d device
        device = dk::DeviceMaker{}.create();

        // Create the main queue
        queue = dk::QueueMaker{device}.setFlags(DkQueueFlags_Graphics).create();

        // Create the memory pools
        pool_images.emplace(device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 16*1024*1024);
        pool_data.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 1*1024*1024);

        // Create the static command buffer and feed it freshly allocated memory
        cmdbuf = dk::CmdBufMaker{device}.create();
        CMemPool::Handle cmdmem = pool_data->allocate(StaticCmdSize);
        cmdbuf.addMemory(cmdmem.getMemBlock(), cmdmem.getOffset(), cmdmem.getSize());

        // Create the framebuffer resources
        createFramebufferResources();

        // Initialize gamepad
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
    }

    ~CExample01()
    {
        // Destroy the framebuffer resources
        destroyFramebufferResources();
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
        // Calculate several measurements for the scene
        unsigned HalfWidth = FramebufferWidth/2, HalfHeight = FramebufferHeight/2;
        unsigned BoxSize = 400;
        unsigned BoxX = HalfWidth - BoxSize/2, BoxY = HalfHeight - BoxSize/2;
        unsigned TileWidth = BoxSize/5, TileHeight = BoxSize/4;

        // Draw a scene using only scissors and clear colors
        cmdbuf.setScissors(0, { { 0, 0, FramebufferWidth, FramebufferHeight } });
        cmdbuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.25f, 0.0f, 1.0f);
        cmdbuf.setScissors(0, { { BoxX, BoxY, BoxSize, BoxSize } });
        cmdbuf.clearColor(0, DkColorMask_RGBA, 229/255.0f, 1.0f, 232/255.0f, 1.0f);
        cmdbuf.setScissors(0, { { BoxX + 2*TileWidth, BoxY + 1*TileHeight, 1*TileWidth, 1*TileHeight } });
        cmdbuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.5f, 0.0f, 1.0f);
        cmdbuf.setScissors(0, { { BoxX + 1*TileWidth, BoxY + 2*TileHeight, 3*TileWidth, 1*TileHeight } });
        cmdbuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.5f, 0.0f, 1.0f);
        render_cmdlist = cmdbuf.finishList();
    }

    void render()
    {
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

        render();
        return true;
    }
};

void Example01(void)
{
    CExample01 app;
    app.run();
}
