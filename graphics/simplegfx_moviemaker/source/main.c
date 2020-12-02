// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

#ifdef DISPLAY_IMAGE
#include "image_bin.h"//Your own raw RGB888 1280x720 image at "data/image.bin" is required.
#endif

// See also libnx display/framebuffer.h.

// This example shows how to use grc MovieMaker, see also grc.h (and applet.h for the requirements for using this).

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720

static int s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock < 0)
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

void userAppInit()
{
    initNxLink();
}

void userAppExit()
{
    deinitNxLink();
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0;
    GrcMovieMaker maker={0};
    GrcOffscreenRecordingParameter makerparam={0};
    u8 *audiobuf = NULL;
    size_t audiobuf_size = (48000/60)*2; // PCM16 samples for a single frame.

    // Initialize and start video recording, the app will immediately exit if this fails.
    rc = grcCreateMovieMaker(&maker, GRC_MOVIEMAKER_WORKMEMORY_SIZE_DEFAULT);
    printf("grcCreateMovieMaker(): 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        grcCreateOffscreenRecordingParameter(&makerparam);

        rc = grcMovieMakerStart(&maker, &makerparam);
        printf("grcMovieMakerStart(): 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        audiobuf = (u8*)malloc(audiobuf_size);
        if (audiobuf)
            memset(audiobuf, 0, audiobuf_size);
        else {
            printf("Failed to allocate audiobuf.\n");
            rc = 1; // Trigger the below R_FAILED block.
        }
    }

    if (R_FAILED(rc)) {
        grcMovieMakerClose(&maker);
        return EXIT_FAILURE;
    }

    // Retrieve the default window + MovieMaker window.
    NWindow* win = nwindowGetDefault();
    NWindow* win_movie = grcMovieMakerGetNWindow(&maker);

    // Create a linear double-buffered framebuffer.
    Framebuffer fb;
    framebufferCreate(&fb, win, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    // Create a linear double-buffered framebuffer, for MovieMaker.
    Framebuffer fb_movie;
    framebufferCreate(&fb_movie, win_movie, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb_movie);

#ifdef DISPLAY_IMAGE
    u8* imageptr = (u8*)image_bin;
#endif

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    u32 cnt = 0;

    // Main loop
    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Retrieve the framebuffer.
        u32 stride;
        u32* framebuf = (u32*) framebufferBegin(&fb, &stride);

        if (cnt != 60)
            cnt ++;
        else
            cnt = 0;

        // Each pixel is 4-bytes due to RGBA8888.
        for (u32 y = 0; y < FB_HEIGHT; y ++)
        {
            for (u32 x = 0; x < FB_WIDTH; x ++)
            {
                u32 pos = y * stride / sizeof(u32) + x;
#ifdef DISPLAY_IMAGE
                framebuf[pos] = RGBA8_MAXALPHA(imageptr[pos*3+0]+(cnt*4), imageptr[pos*3+1], imageptr[pos*3+2]);
#else
                framebuf[pos] = 0x01010101 * cnt * 4;//Set framebuf to different shades of grey.
#endif
            }
        }

        // Retrieve the MovieMaker framebuffer.
        u32* framebuf_movie = (u32*) framebufferBegin(&fb_movie, NULL); // Not using stride since we're just doing memcpy from the above image.

        // Copy the above rendered image to the MovieMaker fb.
        memcpy(framebuf_movie, framebuf, FB_HEIGHT * stride);

        // We're done rendering with MovieMaker, so we end the frame here.
        framebufferEnd(&fb_movie);

        // We're done rendering, so we end the frame here.
        framebufferEnd(&fb);

        // If you want audio you should fill audiobuf with actual data, but here we'll leave it at 0.
        // If you don't use grcMovieMakerEncodeAudioSample, the recorded video will be missing audio.
        rc = grcMovieMakerEncodeAudioSample(&maker, audiobuf, audiobuf_size);
        if (R_FAILED(rc)) printf("grcMovieMakerEncodeAudioSample(): 0x%x\n", rc);
    }

    framebufferClose(&fb);
    framebufferClose(&fb_movie);

    // Finish video recording.
    // You can set your own UserData/thumbnail if you want, but in this example we won't do that.
    // If you want the output caps-entry you can do so, however see grc.h regarding that.

    rc = grcMovieMakerFinish(&maker, 1280, 720, NULL, 0, NULL, 0, NULL);
    printf("grcMovieMakerFinish(): 0x%x\n", rc);

    rc = grcMovieMakerGetError(&maker);
    printf("grcMovieMakerGetError(): 0x%x\n", rc);

    grcMovieMakerClose(&maker);
    free(audiobuf);

    return 0;
}
