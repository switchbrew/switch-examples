#include <string.h>
#include <stdio.h>

#include <switch.h>

//See also libnx hid.h.

int main(int argc, char **argv)
{
    consoleInit(NULL);

    printf(CONSOLE_ESC(1;1H) "Press PLUS to exit.");

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        HidControllerColors colors;

        hidGetControllerColors(CONTROLLER_P1_AUTO, &colors);

        printf(CONSOLE_ESC(2;1H) "singleSet = %d, splitSet = %d.\n", colors.singleSet, colors.splitSet);
        //Note that if a color field is zero that indicates that the color field isn't set, regardless of the *Set fields.

        if (colors.singleSet) {
            printf(CONSOLE_ESC(3;1H) "singleColorBody = 0x%08x, singleColorButtons = 0x%08x.\n", colors.singleColorBody, colors.singleColorButtons);
        }
        else {
            printf(CONSOLE_ESC(3;1H) CONSOLE_ESC(2K));
        }

        if (colors.splitSet) {
            printf(CONSOLE_ESC(4;1H) "leftColorBody = 0x%08x, leftColorButtons = 0x%08x.\n", colors.leftColorBody, colors.leftColorButtons);
            printf(CONSOLE_ESC(5;1H) "rightColorBody = 0x%08x, rightColorButtons = 0x%08x.\n", colors.rightColorBody, colors.rightColorButtons);
        }
        else {
            printf(CONSOLE_ESC(4;1H) CONSOLE_ESC(2K));
            printf(CONSOLE_ESC(5;1H) CONSOLE_ESC(2K));
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
