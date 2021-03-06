/*
** deko3d Example 08: Deferred Shading (Multipass Rendering with Tiled Cache)
** This example shows how to perform deferred shading, a multipass rendering technique that goes well with the tiled cache.
** New concepts in this example:
** - Rendering to multiple render targets (MRT) at once
** - Floating point render targets
** - Enabling and configuring the tiled cache
** - Using the tiled barrier for relaxing ordering to the tiles generated by the binner (as opposed to a full fragment barrier)
** - Custom composition step reading the output of previous rendering passes as textures
*/

// Sample Framework headers
#include "SampleFramework/CApplication.h"
#include "SampleFramework/CMemPool.h"
#include "SampleFramework/CShader.h"
#include "SampleFramework/CCmdMemRing.h"
#include "SampleFramework/CDescriptorSet.h"
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

class CExample08 final : public CApplication
{
    static constexpr unsigned NumFramebuffers = 2;
    static constexpr unsigned StaticCmdSize = 0x10000;
    static constexpr unsigned DynamicCmdSize = 0x10000;
    static constexpr unsigned MaxImages = 3;
    static constexpr unsigned MaxSamplers = 1;

    PadState pad;

    dk::UniqueDevice device;
    dk::UniqueQueue queue;

    std::optional<CMemPool> pool_images;
    std::optional<CMemPool> pool_code;
    std::optional<CMemPool> pool_data;

    dk::UniqueCmdBuf cmdbuf;
    dk::UniqueCmdBuf dyncmd;
    CCmdMemRing<NumFramebuffers> dynmem;

    CDescriptorSet<MaxImages> imageDescriptorSet;
    CDescriptorSet<MaxSamplers> samplerDescriptorSet;

    CShader vertexShader;
    CShader fragmentShader;

    CShader compositionVertexShader;
    CShader compositionFragmentShader;

    Transformation transformState;
    CMemPool::Handle transformUniformBuffer;

    Lighting lightingState;
    CMemPool::Handle lightingUniformBuffer;

    CMemPool::Handle vertexBuffer;
    CMemPool::Handle indexBuffer;

    uint32_t framebufferWidth;
    uint32_t framebufferHeight;

    CMemPool::Handle albedoBuffer_mem;
    CMemPool::Handle normalBuffer_mem;
    CMemPool::Handle viewDirBuffer_mem;
    CMemPool::Handle depthBuffer_mem;
    CMemPool::Handle framebuffers_mem[NumFramebuffers];

    dk::Image albedoBuffer;
    dk::Image normalBuffer;
    dk::Image viewDirBuffer;
    dk::Image depthBuffer;
    dk::Image framebuffers[NumFramebuffers];
    DkCmdList framebuffer_cmdlists[NumFramebuffers];
    dk::UniqueSwapchain swapchain;

    DkCmdList render_cmdlist, composition_cmdlist;

public:
    CExample08()
    {
        // Create the deko3d device
        device = dk::DeviceMaker{}.create();

        // Create the main queue
        queue = dk::QueueMaker{device}.setFlags(DkQueueFlags_Graphics).create();

        // Create the memory pools
        pool_images.emplace(device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 64*1024*1024);
        pool_code.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code, 1*1024*1024);
        pool_data.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 1*1024*1024);

        // Create the static command buffer and feed it freshly allocated memory
        cmdbuf = dk::CmdBufMaker{device}.create();
        CMemPool::Handle cmdmem = pool_data->allocate(StaticCmdSize);
        cmdbuf.addMemory(cmdmem.getMemBlock(), cmdmem.getOffset(), cmdmem.getSize());

        // Create the dynamic command buffer and allocate memory for it
        dyncmd = dk::CmdBufMaker{device}.create();
        dynmem.allocate(*pool_data, DynamicCmdSize);

        // Create the image and sampler descriptor sets
        imageDescriptorSet.allocate(*pool_data);
        samplerDescriptorSet.allocate(*pool_data);

        // Load the shaders
        vertexShader.load(*pool_code, "romfs:/shaders/transform_normal_vsh.dksh");
        fragmentShader.load(*pool_code, "romfs:/shaders/basic_deferred_fsh.dksh");
        compositionVertexShader.load(*pool_code, "romfs:/shaders/composition_vsh.dksh");
        compositionFragmentShader.load(*pool_code, "romfs:/shaders/composition_fsh.dksh");

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

        // Configure persistent state in the queue
        {
            // Bind the image and sampler descriptor sets
            imageDescriptorSet.bindForImages(cmdbuf);
            samplerDescriptorSet.bindForSamplers(cmdbuf);

            // Enable the tiled cache
            cmdbuf.setTileSize(64, 64); // example size, please experiment with this
            cmdbuf.tiledCacheOp(DkTiledCacheOp_Enable);

            // Submit the configuration commands to the queue
            queue.submitCommands(cmdbuf.finishList());
            queue.waitIdle();
            cmdbuf.clear();
        }

        // Initialize gamepad
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
    }

    void createFramebufferResources()
    {
        // Calculate layout for the different buffers part of the g-buffer
        dk::ImageLayout layout_gbuffer;
        dk::ImageLayoutMaker{device}
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_RGBA16_Float)
            .setDimensions(framebufferWidth, framebufferHeight)
            .initialize(layout_gbuffer);

        // Calculate layout for the depth buffer
        dk::ImageLayout layout_depthbuffer;
        dk::ImageLayoutMaker{device}
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_Z24S8)
            .setDimensions(framebufferWidth, framebufferHeight)
            .initialize(layout_depthbuffer);

        // Calculate layout for the framebuffers
        dk::ImageLayout layout_framebuffer;
        dk::ImageLayoutMaker{device}
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent)
            .setFormat(DkImageFormat_RGBA8_Unorm_sRGB)
            .setDimensions(framebufferWidth, framebufferHeight)
            .initialize(layout_framebuffer);

        // Create the albedo buffer
        albedoBuffer_mem = pool_images->allocate(layout_gbuffer.getSize(), layout_gbuffer.getAlignment());
        albedoBuffer.initialize(layout_gbuffer, albedoBuffer_mem.getMemBlock(), albedoBuffer_mem.getOffset());

        // Create the normal buffer
        normalBuffer_mem = pool_images->allocate(layout_gbuffer.getSize(), layout_gbuffer.getAlignment());
        normalBuffer.initialize(layout_gbuffer, normalBuffer_mem.getMemBlock(), normalBuffer_mem.getOffset());

        // Create the view direction buffer
        viewDirBuffer_mem = pool_images->allocate(layout_gbuffer.getSize(), layout_gbuffer.getAlignment());
        viewDirBuffer.initialize(layout_gbuffer, viewDirBuffer_mem.getMemBlock(), viewDirBuffer_mem.getOffset());

        // Create the depth buffer
        depthBuffer_mem = pool_images->allocate(layout_depthbuffer.getSize(), layout_depthbuffer.getAlignment());
        depthBuffer.initialize(layout_depthbuffer, depthBuffer_mem.getMemBlock(), depthBuffer_mem.getOffset());

        // Create the framebuffers
        uint64_t fb_size  = layout_framebuffer.getSize();
        uint32_t fb_align = layout_framebuffer.getAlignment();
        DkImage const* fb_array[NumFramebuffers];
        for (unsigned i = 0; i < NumFramebuffers; i ++)
        {
            // Allocate a framebuffer
            framebuffers_mem[i] = pool_images->allocate(fb_size, fb_align);
            framebuffers[i].initialize(layout_framebuffer, framebuffers_mem[i].getMemBlock(), framebuffers_mem[i].getOffset());

            // Generate a command list that binds the framebuffer
            dk::ImageView framebufferView { framebuffers[i] };
            cmdbuf.bindRenderTargets(&framebufferView);
            framebuffer_cmdlists[i] = cmdbuf.finishList();

            // Fill in the array for use later by the swapchain creation code
            fb_array[i] = &framebuffers[i];
        }

        // Create the swapchain using the framebuffers
        swapchain = dk::SwapchainMaker{device, nwindowGetDefault(), fb_array, NumFramebuffers}.create();

        // Generate the static command lists
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

        // Destroy the rendertargets
        depthBuffer_mem.destroy();
        viewDirBuffer_mem.destroy();
        normalBuffer_mem.destroy();
        albedoBuffer_mem.destroy();
    }

    ~CExample08()
    {
        // Destory the framebuffer resources
        destroyFramebufferResources();

        // Destroy the index buffer (not strictly needed in this case)
        indexBuffer.destroy();

        // Destroy the vertex buffer (not strictly needed in this case)
        vertexBuffer.destroy();

        // Destroy the uniform buffers (not strictly needed in this case)
        lightingUniformBuffer.destroy();
        transformUniformBuffer.destroy();
    }

    void recordStaticCommands()
    {
        // Initialize state structs with deko3d defaults
        dk::RasterizerState rasterizerState;
        dk::ColorState colorState;
        dk::ColorWriteState colorWriteState;
        dk::DepthStencilState depthStencilState;

        // Bind g-buffer and depth buffer
        dk::ImageView albedoTarget { albedoBuffer }, normalTarget { normalBuffer }, viewDirTarget { viewDirBuffer }, depthTarget { depthBuffer };
        cmdbuf.bindRenderTargets({ &albedoTarget, &normalTarget, &viewDirTarget }, &depthTarget);

        // Configure viewport and scissor
        const DkViewport viewport = { 0.0f, 0.0f, float(framebufferWidth), float(framebufferHeight), 0.0f, 1.0f };
        const DkScissor scissor   = { 0, 0, framebufferWidth, framebufferHeight };
        cmdbuf.setViewports(0, { viewport, viewport, viewport });
        cmdbuf.setScissors(0, { scissor, scissor, scissor });

        // Clear the g-buffer and the depth buffer
        cmdbuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);
        cmdbuf.clearColor(1, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);
        cmdbuf.clearColor(2, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);
        cmdbuf.clearDepthStencil(true, 1.0f, 0xFF, 0);

        // Bind state required for drawing the mesh
        cmdbuf.bindShaders(DkStageFlag_GraphicsMask, { vertexShader, fragmentShader });
        cmdbuf.bindUniformBuffer(DkStage_Vertex, 0, transformUniformBuffer.getGpuAddr(), transformUniformBuffer.getSize());
        cmdbuf.bindRasterizerState(rasterizerState);
        cmdbuf.bindColorState(colorState);
        cmdbuf.bindColorWriteState(colorWriteState);
        cmdbuf.bindDepthStencilState(depthStencilState);
        cmdbuf.bindVtxBuffer(0, vertexBuffer.getGpuAddr(), vertexBuffer.getSize());
        cmdbuf.bindVtxAttribState(VertexAttribState);
        cmdbuf.bindVtxBufferState(VertexBufferState);
        cmdbuf.bindIdxBuffer(DkIdxFormat_Uint16, indexBuffer.getGpuAddr());

        // Draw the mesh
        cmdbuf.drawIndexed(DkPrimitive_Triangles, indexBuffer.getSize() / sizeof(u16), 1, 0, 0, 0);

        // Tiled barrier (similar to using Vulkan's vkCmdNextSubpass) + image cache
        // flush so that the next rendering step can access the output from this step
        cmdbuf.barrier(DkBarrier_Tiles, DkInvalidateFlags_Image);

        // Discard the depth buffer since we don't need it anymore
        cmdbuf.discardDepthStencil();

        // End of the main rendering command list
        render_cmdlist = cmdbuf.finishList();

        // Upload image descriptors
        std::array<dk::ImageDescriptor, MaxImages> descriptors;
        descriptors[0].initialize(albedoTarget);
        descriptors[1].initialize(normalTarget);
        descriptors[2].initialize(viewDirTarget);
        imageDescriptorSet.update(cmdbuf, 0, descriptors);

        // Upload sampler descriptor
        dk::Sampler sampler;
        dk::SamplerDescriptor samplerDescriptor;
        samplerDescriptor.initialize(sampler);
        samplerDescriptorSet.update(cmdbuf, 0, samplerDescriptor);

        // Flush the descriptor cache
        cmdbuf.barrier(DkBarrier_None, DkInvalidateFlags_Descriptors);

        // Bind state required for doing the composition
        cmdbuf.setViewports(0, viewport);
        cmdbuf.setScissors(0, scissor);
        cmdbuf.bindShaders(DkStageFlag_GraphicsMask, { compositionVertexShader, compositionFragmentShader });
        cmdbuf.bindUniformBuffer(DkStage_Fragment, 0, lightingUniformBuffer.getGpuAddr(), lightingUniformBuffer.getSize());
        cmdbuf.bindTextures(DkStage_Fragment, 0, {
            dkMakeTextureHandle(0, 0),
            dkMakeTextureHandle(1, 0),
            dkMakeTextureHandle(2, 0),
        });
        cmdbuf.bindRasterizerState(dk::RasterizerState{});
        cmdbuf.bindColorState(dk::ColorState{});
        cmdbuf.bindColorWriteState(dk::ColorWriteState{});
        cmdbuf.bindVtxAttribState({});

        // Draw the full screen quad
        cmdbuf.draw(DkPrimitive_Quads, 4, 1, 0, 0);

        // Tiled barrier
        cmdbuf.barrier(DkBarrier_Tiles, 0);

        // Discard the g-buffer since we don't need it anymore
        cmdbuf.bindRenderTargets({ &albedoTarget, &normalTarget, &viewDirTarget });
        cmdbuf.discardColor(0);
        cmdbuf.discardColor(1);
        cmdbuf.discardColor(2);

        // End of the composition cmdlist
        composition_cmdlist = cmdbuf.finishList();
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

        // Submit the command list that binds the correct framebuffer
        queue.submitCommands(framebuffer_cmdlists[slot]);

        // Submit the command list used for performing the composition
        queue.submitCommands(composition_cmdlist);

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
        //   mdlvMtx = Translate * RotateX * RotateY * Translate
        // This means that the Scale operation is applied first, then RotateY, and so on.
        transformState.mdlvMtx = glm::mat4{1.0f};
        transformState.mdlvMtx = glm::translate(transformState.mdlvMtx, glm::vec3{sinf(period1*tau), 0.0f, -3.0f});
        transformState.mdlvMtx = glm::rotate(transformState.mdlvMtx, sinf(period2 * tau) * tau / 8.0f, glm::vec3{1.0f, 0.0f, 0.0f});
        transformState.mdlvMtx = glm::rotate(transformState.mdlvMtx, -period1 * tau, glm::vec3{0.0f, 1.0f, 0.0f});
        transformState.mdlvMtx = glm::translate(transformState.mdlvMtx, glm::vec3{0.0f, -0.5f, 0.0f});

        render();
        return true;
    }
};

void Example08(void)
{
    CExample08 app;
    app.run();
}
