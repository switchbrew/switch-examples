#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <switch.h>

//Joy-con IR-sensor example, displays the image from the IR camera.
//The Joy-con must be detached from the system.

int main(int argc, char **argv)
{
    Result rc=0;
    Result rc2=0;
    u32 irhandle=0;
    IrsImageTransferProcessorConfig config;
    IrsImageTransferProcessorState state;
    size_t ir_buffer_size = 0x12c00;
    u8 *ir_buffer = NULL;

    u32 width, height;
    u32 ir_width, ir_height;
    u32 pos, pos2=0;
    u32* framebuf;

    gfxInitDefault();

    //Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
    consoleInit(NULL);

    ir_buffer = (u8*)malloc(ir_buffer_size);
    if (ir_buffer==NULL)
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        printf("Failed to allocate memory for ir_buffer.\n");
    }
    else
    {
        memset(ir_buffer, 0, ir_buffer_size);
    }

    if (R_SUCCEEDED(rc))
    {
        rc = irsInitialize();
        printf("irsInitialize() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        rc = irsActivateIrsensor(1);
        printf("irsActivateIrsensor() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        rc = irsGetIrCameraHandle(&irhandle, CONTROLLER_PLAYER_1);
        //If you want to use handheld-mode/non-CONTROLLER_PLAYER_* for irsensor you have to set irhandle directly, for example: irhandle = CONTROLLER_HANDHELD;
        printf("irsGetIrCameraHandle() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        irsGetDefaultImageTransferProcessorConfig(&config);
        rc = irsRunImageTransferProcessor(irhandle, &config, 0x100000);
        printf("irsRunImageTransferProcessor() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        //Switch to using regular framebuffer.
        consoleClear();
        gfxSetMode(GfxMode_LinearDouble);
    }

    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        if (R_SUCCEEDED(rc))
        {
            framebuf = (u32*) gfxGetFramebuffer((u32*)&width, (u32*)&height);

            //Note that the image is updated every few seconds. Likewise, it takes a few seconds for the initial image to become available.
            //This will return an error when no image is available yet.
            rc2 = irsGetImageTransferProcessorState(irhandle, ir_buffer, ir_buffer_size, &state);

            if (R_SUCCEEDED(rc2))
            {
                memset(framebuf, 0, gfxGetFramebufferSize());

                //IR image width/height with the default config.
                //The image is grayscale (1 byte per pixel / 8bits, with 1 color-component).
                ir_width = 240;
                ir_height = 320;

                u32 x, y;
                for (y=0; y<ir_height; y++)//Access the buffer linearly.
                {
                    for (x=0; x<ir_width; x++)
                    {
                        pos = y * width + x;
                        pos2 = x * ir_height + y;//The IR image/camera is sideways with the joycon held flat.
                        framebuf[pos] = RGBA8_MAXALPHA(/*ir_buffer[pos2]*/0, ir_buffer[pos2], /*ir_buffer[pos2]*/0);
                    }
                }
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    irsStopImageProcessor(irhandle);
    irsExit();

    free(ir_buffer);

    gfxExit();
    return 0;
}

