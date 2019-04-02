// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the Parental Controls service / auth LibraryApplet, see also libnx pctl.h/pctlauth.h.

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0;
    bool pctlflag=0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("pctlauth example\n");
    consoleUpdate(NULL);

    rc = pctlInitialize();
    printf("pctlInitialize(): 0x%x\n", rc);
    if (R_SUCCEEDED(rc)) {
        rc = pctlIsRestrictionEnabled(&pctlflag);
        pctlExit();
        printf("pctlIsRestrictionEnabled(): 0x%x", rc);
        if (R_SUCCEEDED(rc)) printf(", flag=%d", pctlflag);
        printf("\n");
    }

    if (pctlflag) {
        printf("Press A to launch pctlauth applet.\n");
    }
    else {
        printf("Parental Controls are not enabled, hence applet launching is disabled.\n");
    }
    printf("Press + to exit.\n");

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

        if (pctlflag && (kDown & KEY_A)) {
            printf("Running pctlauthShow()...\n");
            consoleUpdate(NULL);
            rc = pctlauthShow(true); // Launch the applet for validating the PIN.
            printf("pctlauthShow(): 0x%x\n", rc);
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
