#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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

__attribute__((format(printf, 1, 2)))
static int error_screen(const char* fmt, ...)
{
    consoleInit(NULL);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("Press PLUS to exit\n");
    while (appletMainLoop())
    {
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
            break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    Result rc=0;

    const size_t ir_buffer_size = 0x12c00; // Size for the max IrsImageTransferProcessorFormat.
    u8 *ir_buffer = NULL;
    ir_buffer = (u8*)malloc(ir_buffer_size);
    if (!ir_buffer) {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        return error_screen("Failed to allocate memory for ir_buffer.\n");
    }

    memset(ir_buffer, 0, ir_buffer_size);

    // Get the handle for the specified controller.
    IrsIrCameraHandle irhandle;
    hidScanInput(); // Only needed because hidScanInput() was not used yet since this is before the main-loop, and we're using hidGetHandheldMode().
    rc = irsGetIrCameraHandle(&irhandle, hidGetHandheldMode() ? CONTROLLER_HANDHELD : CONTROLLER_PLAYER_1);
    if (R_FAILED(rc))
        return error_screen("irsGetIrCameraHandle() returned 0x%x\n", rc);

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
        return error_screen("irsRunImageTransferProcessor() returned 0x%x\n", rc);

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    u64 sampling_number=0;

    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

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
