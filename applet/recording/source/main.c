#include <string.h>
#include <stdio.h>
#include <switch.h>

//See also libnx applet.h. See applet.h for the requirements for using this.

extern u32 g_appletRecordingInitialized;

int main(int argc, char **argv)
{
    u32* framebuf;
    u32  cnt=0;

    gfxInitDefault();

    appletInitializeGamePlayRecording();//Normally this is only recording func you need to call.

    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        if (kDown & KEY_Y) {
            appletSetGamePlayRecordingState(0);//Disable recording.
        }
        else if (kDown & KEY_X) {
            appletSetGamePlayRecordingState(1);//Enable recording.
        }

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
                framebuf[pos] = 0x01010101 * cnt * 4;//Set framebuf to different shades of grey.
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
    }

    gfxExit();
    return 0;
}
