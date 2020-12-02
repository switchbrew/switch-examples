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
    printf(CONSOLE_ESC(42;37m));
    printf(CONSOLE_ESC( 7;4H) "%s", "     _      _        ____      _ ");
    printf(CONSOLE_ESC( 8;4H) "%s", "    | |    | |      |___ \\    | |");
    printf(CONSOLE_ESC( 9;4H) "%s", "  __| | ___| | _____  __) | __| |");
    printf(CONSOLE_ESC(10;4H) "%s", " / _` |/ _ \\ |/ / _ \\|__ < / _` |");
    printf(CONSOLE_ESC(11;4H) "%s", "| (_| |  __/   < (_) |__) | (_| |");
    printf(CONSOLE_ESC(12;4H) "%s", " \\__,_|\\___|_|\\_\\___/____/ \\__,_|");
    printf(CONSOLE_ESC(0m));

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
        printf(    CONSOLE_ESC(%1$dm) /* Set color */
                "Default "
                CONSOLE_ESC(1m) "Bold "
                CONSOLE_ESC(7m) "Reversed "

                CONSOLE_ESC(0m) /* revert attributes*/
                CONSOLE_ESC(%1$dm)

                CONSOLE_ESC(2m) "Light "
                CONSOLE_ESC(7m) "Reversed "

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
