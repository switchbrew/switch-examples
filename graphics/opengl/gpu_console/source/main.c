#include <string.h>
#include <stdio.h>

#include <switch.h>

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

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
                CONSOLE_ESC(2m) "Light "
                "\n"
                CONSOLE_ESC(0m) /* revert attributes*/
                , i + 30);
    }

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // Your code goes here

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
