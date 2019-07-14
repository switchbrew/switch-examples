// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use applet ApplicationCopyright (used as a screenshot overlay), see also libnx applet.h.

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("screenshot-overlay example\n");

    // Setup the RGBA8 image.
    Result rc=0;
    size_t size = 128*128*4;
    u8 *tmpbuf = malloc(size);
    if (tmpbuf)
        memset(tmpbuf, 0xff, size); // Set the image to all white.
    else {
        rc = 1;
        printf("Failed to allocate memory.\n");
    }

    // Initialize.
    if (R_SUCCEEDED(rc)) {
        rc = appletInitializeApplicationCopyrightFrameBuffer();
        printf("appletInitializeApplicationCopyrightFrameBuffer(): 0x%x\n", rc);
    }

    // Set the image.
    if (R_SUCCEEDED(rc)) {
        rc = appletSetApplicationCopyrightImage(tmpbuf, size, 0, 0, 128, 128, 1);
        printf("appletSetApplicationCopyrightImage(): 0x%x\n", rc);
    }

    // Set the overlay visibility in the screenshot. This is set to true by default, so normally this is only needed for disabling/re-enabling visibility.
    if (R_SUCCEEDED(rc)) {
        rc = appletSetApplicationCopyrightVisibility(true);
        printf("appletSetApplicationCopyrightVisibility(): 0x%x\n", rc);
    }

    free(tmpbuf);

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

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
