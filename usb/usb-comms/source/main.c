// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// Function just to wait the user to type + to continue the code.
// Useful to give a time to user read the console.
void waitToTypePlus()
{
  printf("Type + to leave\n");
  while (appletMainLoop())
  {
      hidScanInput();

      u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

      if (kDown & KEY_PLUS)
          break;

      consoleUpdate(NULL);
  }
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("Trying to initialize the usb interface...\n");

    Result rc = usbCommsInitialize();

    if (R_FAILED(rc)) {
        printf("Fail to initialize usb!\n");
        waitToTypePlus();
        consoleExit(NULL);
        return 0;
    }

    printf("Success on initialize usb!\n");
    printf("Waiting to receive a payload from browser...\n");
    consoleUpdate(NULL);

    char text[3] = {0, 0, 0};
    usbCommsRead(text, 3);
    printf("Payload received: %X %X %X\n", text[0], text[1], text[2]);

    printf("Press a A or B to send to browser...\n");
    consoleUpdate(NULL);

    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_A) {
          char payload[1] = { 'A' };
          usbCommsWrite(payload, 1);
          break;
        }

        if (kDown & KEY_B) {
          char payload[1] = { 'B' };
          usbCommsWrite(payload, 1);
          break;
        }
    }

    printf("Payload sent!\n");

    usbCommsExit();

    waitToTypePlus();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
