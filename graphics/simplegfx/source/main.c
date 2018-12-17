// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

#ifdef DISPLAY_IMAGE
#include "image_bin.h"//Your own raw RGB888 1280x720 image at "data/image.bin" is required.
#endif

// See also libnx display/framebuffer.h.

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720

// Remove above and uncomment below for 1080p
//#define FB_WIDTH  1920
//#define FB_HEIGHT 1080

// Main program entrypoint
int main(int argc, char* argv[])
{
    // Retrieve the default window
    NWindow* win = nwindowGetDefault();

    // Create a linear double-buffered framebuffer
    Framebuffer fb;
    framebufferCreate(&fb, win, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

#ifdef DISPLAY_IMAGE
    u8* imageptr = (u8*)image_bin;
#endif

    u32 cnt = 0;

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        // Retrieve the framebuffer
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

        // We're done rendering, so we end the frame here.
        framebufferEnd(&fb);
    }

    framebufferClose(&fb);
    return 0;
}
