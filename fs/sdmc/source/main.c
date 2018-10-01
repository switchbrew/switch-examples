#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <switch.h>

//The SD card is automatically mounted as the default device, usable with standard stdio. SD root dir is located at "/" (also "sdmc:/" but normally using the latter isn't needed).
//The default current-working-directory when using relative paths is normally the directory where your application is located on the SD card.

int main(int argc, char **argv)
{
    consoleInit(NULL);

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
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
