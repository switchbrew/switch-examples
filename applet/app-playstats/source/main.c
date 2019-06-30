// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use applet to get playstats for applications. See also libnx applet.h. See applet.h for the requirements for using this.

/// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("applet application play-stats example\n");

    Result rc=0;
    PdmApplicationPlayStatistics stats[1] = {0};
    u64 titleIDs[1] = {0x010021B002EEA000}; // Change this to the titleID of the current-process / the titleID you want to use.
    s32 total_out=0;
    s32 i;
    s32 i2;

    // Use appletQueryApplicationPlayStatisticsByUid if you want playstats for a specific userID.

    rc = appletQueryApplicationPlayStatistics(stats, titleIDs, sizeof(titleIDs)/sizeof(u64), &total_out);
    printf("appletQueryApplicationPlayStatisticsByUid(): 0x%x\n", rc);
    if (R_SUCCEEDED(rc)) {
        printf("total_out: %d\n", total_out);
        for (i=0; i<total_out; i++) {
            printf("%d: ", i);
            printf("titleID = 0x%08lX ", stats[i].titleID);
            for (i2=0; i2<sizeof(stats[i].unk_x8); i2++) printf("%02X ", stats[i].unk_x8[i2]);
            printf("\n");
        }
    }

    // Main loop
    while (appletMainLoop())//This loop will automatically exit when applet requests exit.
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        // Your code goes here

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
