#include <string.h>
#include <stdio.h>

#include <switch.h>

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // clear screen and home cursor
    printf( CONSOLE_ESC(2J) );

    // Set print co-ordinates
    // /x1b[row;columnH
    printf(CONSOLE_ESC(10;10H) "VT52 codes demo");

    // move cursor up
    // /x1b[linesA
    printf(CONSOLE_ESC(10A)"Line 0");

    // move cursor left
    // /x1b[columnsD
    printf(CONSOLE_ESC(28D)"Column 0");

    // move cursor down
    // /x1b[linesB
    printf(CONSOLE_ESC(19B)"Line 19");

    // move cursor right
    // /x1b[columnsC
    printf(CONSOLE_ESC(5C)"Column 20");

    printf("\n");

    // Color codes and attributes
    for(int i=0; i<8; i++)
    {
        printf(    CONSOLE_ESC(%1$d;1m) /* Set color */
                "Default "
                CONSOLE_ESC(1m) "Bold "
                CONSOLE_ESC(7m) "Reversed "

                CONSOLE_ESC(0m) /* revert attributes*/
                CONSOLE_ESC(%1$d;1m)

                CONSOLE_ESC(2m) "Light "
                CONSOLE_ESC(7m) "Reversed "

                CONSOLE_ESC(0m) /* revert attributes*/
                CONSOLE_ESC(%1$d;1m)
                CONSOLE_ESC(4m) "Underline "

                CONSOLE_ESC(0m) /* revert attributes*/
                CONSOLE_ESC(%1$d;1m)
                CONSOLE_ESC(9m) "Strikethrough "
                "\n"
                CONSOLE_ESC(0m) /* revert attributes*/
                , i + 30);
    }

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // Your code goes here

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
