// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use VrMode, see also libnx applet.h and pctl.h.
// TODO: This needs replaced with GL rendering, to handle the special VrMode rendering.

// When running on pre-7.0.0, hbl has to run under the labo-vr application for the host application.

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc = 0;
    bool pctlinit=0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("VrMode example\n");

    // Using pctl is optional.
    rc = pctlInitialize();
    if (R_FAILED(rc)) printf("pctlInitialize(): 0x%x\n", rc);
    if (R_SUCCEEDED(rc)) pctlinit = 1;

    if (pctlinit) {
        printf("Press A to enable VrMode, B to disable, and X to check VrMode.\n");
    }
    else {
        printf("VrMode functionality disabled since pctlInitialize failed.\n");
    }

    printf("Press + to exit.\n");

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

        if (pctlinit) {
            if (kDown & HidNpadButton_A) {
                // Not sure what this is.
                /*rc = pctlConfirmStereoVisionPermission();
                if (R_FAILED(rc)) printf("pctlConfirmStereoVisionPermission(): 0x%x\n", rc);*/

                bool tmpflag=0;
                rc = pctlIsStereoVisionPermitted(&tmpflag);
                if (R_FAILED(rc)) printf("pctlIsStereoVisionPermitted(): 0x%x\n", rc);

                if (!tmpflag) printf("Parental Controls doesn't allow using VrMode.\n");

                if (R_SUCCEEDED(rc) && tmpflag) {
                    rc = appletSetVrModeEnabled(true);
                    printf("appletSetVrModeEnabled(true): 0x%x\n", rc);
                }
            }
            else if (kDown & HidNpadButton_B) {
                rc = appletSetVrModeEnabled(false);
                printf("appletSetVrModeEnabled(false): 0x%x\n", rc);

                // See above comment.
                /*rc = pctlResetConfirmedStereoVisionPermission();
                printf("pctlResetConfirmedStereoVisionPermission(): 0x%x\n", rc);*/
            }
            else if (kDown & HidNpadButton_X) {
                bool flag=0;
                rc = appletIsVrModeEnabled(&flag);
                printf("appletIsVrModeEnabled(): 0x%x", rc);
                if (R_SUCCEEDED(rc)) printf(", %d\n", flag);
                printf("\n");
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    if (pctlinit) pctlExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
