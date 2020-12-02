/*
  run this example from nxlink

  nxlink <-a switch_ip> nxlink_stdio.nro -s <arguments>

  -s or --server tells nxlink to open a socket nxlink can connect to.

*/

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <unistd.h>

#include <switch.h>

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    // Initialise sockets
    socketInitializeDefault();

    printf("Hello World!\n");

    // Display arguments sent from nxlink
    printf("%d arguments\n", argc);

    for (int i=0; i<argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    // the host ip where nxlink was launched
    printf("nxlink host is %s\n", inet_ntoa(__nxlink_host));

    // redirect stdout & stderr over network to nxlink
    nxlinkStdio();

    // this text should display on nxlink host
    printf("printf output now goes to nxlink server\n");

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // Your code goes here

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u32 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        if (kDown & HidNpadButton_A) {
            printf("A Pressed\n");
        }
        if (kDown & HidNpadButton_B) {
            printf("B Pressed\n");
        }

        consoleUpdate(NULL);
    }

    socketExit();
    consoleExit(NULL);
    return 0;
}
