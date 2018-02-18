#include <string.h>
#include <stdio.h>
#include <switch.h>

#ifdef DISPLAY_IMAGE
#include "image_bin.h"//Your own raw RGB888 1280x720 image at "data/image.bin" is required.
#endif

//See also libnx gfx.h.

int main(int argc, char **argv)
{
    u32* framebuf;
    u32  cnt=0;
    #ifdef DISPLAY_IMAGE
    u8*  imageptr = (u8*)image_bin;
    #endif

    //Enable max-1080p support. Remove for 720p-only resolution.
    //gfxInitResolutionDefault();

    gfxInitDefault();

    //Set current resolution automatically depending on current/changed OperationMode. Only use this when using gfxInitResolution*().
    //gfxConfigureAutoResolutionDefault(true);

    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        u32 width, height;
        u32 pos;
        framebuf = (u32*) gfxGetFramebuffer((u32*)&width, (u32*)&height);

        if(cnt==60)
        {
            cnt=0;
        }
        else
        {
            cnt++;
        }

        //Each pixel is 4-bytes due to RGBA8888.
        u32 x, y;
        for (y=0; y<height; y++)//Access the buffer linearly.
        {
            for (x=0; x<width; x++)
            {
                pos = y * width + x;
                #ifdef DISPLAY_IMAGE
                framebuf[pos] = RGBA8_MAXALPHA(imageptr[pos*3+0]+(cnt*4), imageptr[pos*3+1], imageptr[pos*3+2]);
                #else
                framebuf[pos] = 0x01010101 * cnt * 4;//Set framebuf to different shades of grey.
                #endif
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}
