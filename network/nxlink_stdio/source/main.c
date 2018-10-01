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
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // Your code goes here

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        if (kDown & KEY_A) {
            printf("A Pressed\n");
        }
        if (kDown & KEY_B) {
            printf("B Pressed\n");
        }

        consoleUpdate(NULL);
    }

    socketExit();
    consoleExit(NULL);
    return 0;
}
