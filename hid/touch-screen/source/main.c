// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the touch-screen. See also libnx hid.h.

// Main program entrypoint
int main(int argc, char* argv[])
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

    hidInitializeTouchScreen();

    s32 prev_touchcount=0;

    printf("\x1b[1;1HPress PLUS to exit.");
    printf("\x1b[2;1HTouch Screen position:");

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

        HidTouchScreenState state={0};
        if (hidGetTouchScreenStates(&state, 1)) {
            if (state.count != prev_touchcount)
            {
                prev_touchcount = state.count;

                consoleClear();

                printf("\x1b[1;1HPress PLUS to exit.");
                printf("\x1b[2;1HTouch Screen position:");
            }

            printf("\x1b[3;1H");

            for(s32 i=0; i<state.count; i++)
            {
                // Print the touch screen coordinates
                printf("[%d] x=%03d, y=%03d, diameter_x=%03d, diameter_y=%03d, rotation_angle=%03d\n", i, state.touches[i].x, state.touches[i].y, state.touches[i].diameter_x, state.touches[i].diameter_y, state.touches[i].rotation_angle);
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
