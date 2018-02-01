#include <string.h>
#include <stdio.h>
#include <switch.h>

//See also libnx gfx.h.

int main(int argc, char **argv)
{
    u32* framebuf;
    u32  cnt=0;

    //Enable max-1080p support. Remove for 720p-only resolution.
    //gfxInitResolutionDefault();

    gfxInitDefault();

    //Set current resolution automatically depending on current/changed OperationMode. Only use this when using gfxInitResolution*().
    //gfxConfigureAutoResolutionDefault(true);

    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS) break;

        u32 width, height;
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
        //Set framebuf to different shades of grey.
        u32 x, y;
        for (x=0; x<width; x++)
            for (y=0; y<height; y++)
                framebuf[gfxGetFramebufferDisplayOffset(x, y)] = 0x01010101 * cnt * 4;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}
