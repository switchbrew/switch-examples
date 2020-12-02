// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// See also libnx applet.h.

// Main program entrypoint
int main(int argc, char* argv[])
{
    appletLockExit();//Lock exiting so that the process won't be force-terminated by applet-exit (HOME button / application-close). This allows the app to run cleanup when exiting before getting terminated. For example, this allows closing files / FS operations before termination, etc.
    //However, the process will be force-terminated if appletUnlockExit was not called within 15 seconds after exit was requested.
    //You can also use appletLockExit/appletUnlockExit outside of main() around operations that need it, if you want.

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

    printf("applet exit-locking example.\n");

    // Main loop
    while (appletMainLoop())//This loop will automatically exit when applet requests exit.
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Your code goes here

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);

    svcSleepThread(5000000000ULL);//Sleep for 5 seconds. This shows the additional delay when exiting, actual apps do not need this.

    //You must run cleanup inbetween the loop exiting (or at some point after appletLockExit) and appletUnlockExit.
    appletUnlockExit();//Must be called at some point before main() returns when appletLockExit was used. The process will be terminated when calling this if exit was requested.
    return 0;
}
