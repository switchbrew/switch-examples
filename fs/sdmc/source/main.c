#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <switch.h>

//The SD card is automatically mounted as the default device, usable with standard stdio. SD root dir is located at "/" (also "sdmc:/" but normally using the latter isn't needed).
//The default current-working-directory when using relative paths is normally the directory where your application is located on the SD card.

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    DIR* dir;
    struct dirent* ent;

    dir = opendir("");//Open current-working-directory.
    if(dir==NULL)
    {
        printf("Failed to open dir.\n");
    }
    else
    {
        printf("Dir-listing for '':\n");
        while ((ent = readdir(dir)))
        {
            printf("d_name: %s\n", ent->d_name);
        }
        closedir(dir);
        printf("Done.\n");
    }

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
