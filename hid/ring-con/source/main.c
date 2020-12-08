// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the Ring-Con which attaches to a Joy-Con (from Ring Fit Adventure).
// See also libnx ringcon.h.

// Main program entrypoint
int main(int argc, char **argv)
{
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

    printf(CONSOLE_ESC(1;1H)"Press PLUS to exit.");

    Result rc=0;
    RingCon ring={0};
    bool ready=0;
    rc = ringconCreate(&ring, HidNpadIdType_No1); // Setup Ring-Con usage for the specified controller, if you want to use multiple controllers you can do so by calling this multiple times with multiple RingCon objects/controllers. The Ring-Con must be connected to the specified controller.
    printf(CONSOLE_ESC(2;1H)"ringconCreate(): 0x%x, 0x%x", rc, ringconGetErrorFlags(&ring)); // You can also use ringconGetErrorFlag().
    ready = R_SUCCEEDED(rc);

    // For more ringcon functionality, see ringcon.h.

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Get the sensor state data.
        if (ready) {
            s32 total_out=0;
            RingConPollingData polldata[0x1]; // For this example we'll just read the latest entry, however if you want the rest use 0x9.
            rc = ringconGetPollingData(&ring, polldata, 0x1, &total_out); // See libnx ringcon.h regarding ringconGetPollingData().
            printf(CONSOLE_ESC(4;1H)"ringconGetPollingData(): 0x%x\n", rc);
            printf("total_out=%d\n", total_out);
            for (s32 polli=0; polli<total_out; polli++) printf("[%d]: data =     %d\n", polli, polldata[polli].data);

            // Normally(?) the UserCal is not calibrated. So instead, you have to have the user hold the Ring-Con in various positions, then use that as your app-specific calibration (with the above polling-data).
            // Then afterwards in your app you'd compare your app-specific calibration with the polling-data.
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    ringconClose(&ring);
    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
