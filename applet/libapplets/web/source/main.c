// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// See also libnx web.h.

// This example shows how to use the web LibraryApplets.

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("web example\n");

    consoleUpdate(NULL);

    printf("Press A to launch Web applet.\n");
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

        if (kDown & KEY_A) {
            WebCommonConfig config;
            WebCommonReply reply;
            WebExitReason exitReason=0;

            // Create the config. There's a number of web*Create() funcs, see libnx web.h.
            // webPageCreate/webNewsCreate requires running under a host title which has HtmlDocument content, when the title is an Application. When the title is an Application when using webPageCreate/webNewsCreate, and webConfigSetWhitelist is not used, the whitelist will be loaded from the content.
            rc = webPageCreate(&config, "https://google.com/");
            printf("webPageCreate(): 0x%x\n", rc);

            if (R_SUCCEEDED(rc)) {
                // At this point you can use any webConfigSet* funcs you want.

                rc = webConfigSetWhitelist(&config, "^http*"); // Set the whitelist, adjust as needed. If you're only using a single domain, you could remove this and use webNewsCreate for the above (see web.h for webNewsCreate).
                printf("webConfigSetWhitelist(): 0x%x\n", rc);

                if (R_SUCCEEDED(rc)) { // Launch the applet and wait for it to exit.
                    printf("Running webConfigShow...\n");
                    rc = webConfigShow(&config, &reply); // If you don't use reply you can pass NULL for it.
                    printf("webConfigShow(): 0x%x\n", rc);
                }

                if (R_SUCCEEDED(rc)) { // Normally you can ignore exitReason.
                    rc = webReplyGetExitReason(&reply, &exitReason);
                    printf("webReplyGetExitReason(): 0x%x", rc);
                    if (R_SUCCEEDED(rc)) printf(", 0x%x", exitReason);
                    printf("\n");
                }
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
