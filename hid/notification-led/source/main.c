// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the HOME-button notification-LED, see also libnx hidsys.h.

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    Result rc=0;
    bool initflag=0;
    size_t i;
    size_t total_entries;
    u64 UniquePadIds[2];
    HidsysNotificationLedPattern pattern;

    memset(&pattern, 0, sizeof(pattern));

    // Setup the pattern data.
    pattern.globalMiniCycleDuration = 0xf;
    pattern.totalMiniCycles = 0xf;
    pattern.totalFullCycles = 0xf;
    pattern.startIntensity = 0xf;

    printf("notification-led example\n");

    rc = hidsysInitialize();
    if (R_FAILED(rc)) {
        printf("hidsysInitialize(): 0x%x\n", rc);
        printf("Init failed, press + to exit.\n");
    }
    else {
        initflag = 1;
        printf("Press A to set the notification-LED pattern.\n");
        printf("Press + to exit.\n");
    }

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

        if (kDown & KEY_A) {
            total_entries = 0;
            memset(UniquePadIds, 0, sizeof(UniquePadIds));

            // Get the UniquePadIds for the specified controller, which will then be used with hidsysSetNotificationLedPattern.
            // If you want to get the UniquePadIds for all controllers, you can use hidsysGetUniquePadIds instead.
            rc = hidsysGetUniquePadsFromNpad(hidGetHandheldMode() ? CONTROLLER_HANDHELD : CONTROLLER_PLAYER_1, UniquePadIds, 2, &total_entries);
            printf("hidsysGetUniquePadsFromNpad(): 0x%x", rc);
            if (R_SUCCEEDED(rc)) printf(", %ld", total_entries);
            printf("\n");

            if (R_SUCCEEDED(rc)) {
                for(i=0; i<total_entries; i++) { // System will skip sending the subcommand to controllers where this isn't available.
                    printf("[%ld] = 0x%lx ", i, UniquePadIds[i]);
                    rc = hidsysSetNotificationLedPattern(&pattern, UniquePadIds[i]);
                    printf("hidsysSetNotificationLedPattern(): 0x%x\n", rc);
                }
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    if (initflag) hidsysExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
