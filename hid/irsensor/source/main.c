#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// Joy-Con IR-sensor example, displays the image from the IR camera. See also libnx irs.h.

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720

void userAppInit(void)
{
    Result rc;

    rc = irsInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(rc);
}

void userAppExit(void)
{
    irsExit();
}

__attribute__((format(printf, 2, 3)))
static int error_screen(PadState *pad, const char* fmt, ...)
{
    consoleInit(NULL);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("Press PLUS to exit\n");
    while (appletMainLoop())
    {
        padUpdate(pad);
        if (padGetButtonsDown(pad) & HidNpadButton_Plus)
            break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return EXIT_FAILURE;
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0;

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    const size_t ir_buffer_size = 0x12c00; // Size for the max IrsImageTransferProcessorFormat.
    u8 *ir_buffer = NULL;
    ir_buffer = (u8*)malloc(ir_buffer_size);
    if (!ir_buffer) {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        return error_screen(&pad, "Failed to allocate memory for ir_buffer.\n");
    }

    memset(ir_buffer, 0, ir_buffer_size);

    // Get the handle for the specified controller.
    padUpdate(&pad); // Only needed because this wasn't used yet, and we're using padIsHandheld.
    IrsIrCameraHandle irhandle;
    rc = irsGetIrCameraHandle(&irhandle, padIsHandheld(&pad) ? HidNpadIdType_Handheld : HidNpadIdType_No1);
    if (R_FAILED(rc))
        return error_screen(&pad, "irsGetIrCameraHandle() returned 0x%x\n", rc);

    // If a controller update is needed, force an update.
    bool updateflag=0;
    rc = irsCheckFirmwareUpdateNecessity(irhandle, &updateflag);
    if (R_SUCCEEDED(rc) && updateflag) {
        HidLaControllerFirmwareUpdateArg updatearg;
        hidLaCreateControllerFirmwareUpdateArg(&updatearg);
        updatearg.enable_force_update = 1;
        hidLaShowControllerFirmwareUpdate(&updatearg);
    }

    // Run the ImageTransferProcessor with the default config. The default uses the max IrsImageTransferProcessorFormat which has the slowest update-rate with irsGetImageTransferProcessorState. Hence, you may want to use a different IrsImageTransferProcessorFormat, or trim the image with irsRunImageTransferExProcessor.
    IrsImageTransferProcessorConfig config;
    irsGetDefaultImageTransferProcessorConfig(&config);
    rc = irsRunImageTransferProcessor(irhandle, &config, 0x100000);
    if (R_FAILED(rc))
        return error_screen(&pad, "irsRunImageTransferProcessor() returned 0x%x\n", rc);

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    u64 sampling_number=0;

    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // With the default config the image is updated every few seconds (see above). Likewise, it takes a few seconds for the initial image to become available.
        // This will return an error when no image is available yet.
        IrsImageTransferProcessorState state;
        rc = irsGetImageTransferProcessorState(irhandle, ir_buffer, ir_buffer_size, &state);

        u32 stride;
        u32* framebuf = (u32*)framebufferBegin(&fb, &stride);

        if (R_SUCCEEDED(rc) && state.sampling_number != sampling_number) { // Only update framebuf when irsGetImageTransferProcessorState() is successful, where sampling_number changed.
            sampling_number = state.sampling_number;
            memset(framebuf, 0, stride*FB_HEIGHT);

            // IR image width/height with the default config.
            // The image is grayscale (1 byte per pixel / 8bits, with 1 color-component).
            const u32 ir_width = 320;
            const u32 ir_height = 240;

            u32 x, y;
            for (y=0; y<ir_height; y++) { // Access the buffer linearly.
                for (x=0; x<ir_width; x++) {
                    u32 pos = y * stride/sizeof(u32) + x;
                    u32 pos2 = y * ir_width + x;//The IR image/camera is sideways with the joycon held flat. We won't rotate it here - you can do so yourself if you want.
                    framebuf[pos] = RGBA8_MAXALPHA(/*ir_buffer[pos2]*/0, ir_buffer[pos2], /*ir_buffer[pos2]*/0);
                }
            }
        }

        framebufferEnd(&fb);
    }

    framebufferClose(&fb);
    irsStopImageProcessor(irhandle);
    free(ir_buffer);
    return 0;
}
