#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <switch.h>

//This example shows how to access savedata for (official) applications/games.

int main(int argc, char **argv)
{
    Result rc=0;
    int ret=0;

    DIR* dir;
    struct dirent* ent;

    FsFileSystem tmpfs;
    u128 userID=0;
    bool account_selected=0;
    u64 titleid=0x01007ef00011e000;//titleID of the save to mount, in this case BOTW.

    gfxInitDefault();
    consoleInit(NULL);

    //Get the userID for save mounting. To mount common savedata, use FS_SAVEDATA_USERID_COMMONSAVE.
    rc = accountInitialize();
    if (R_FAILED(rc)) {
        printf("accountInitialize() failed: 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        rc = accountGetActiveUser(&userID, &account_selected);
        accountExit();

        if (R_FAILED(rc)) {
            printf("accountGetActiveUser() failed: 0x%x\n", rc);
        }
        else if(!account_selected) {
            printf("No user is currently selected.\n");
            rc = -1;
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = fsMount_SaveData(&tmpfs, titleid, userID);//See also libnx fs.h.
        if (R_FAILED(rc)) {
            printf("fsMount_SaveData() failed: 0x%x\n", rc);
        }
    }

    if (R_SUCCEEDED(rc)) {
        ret = fsdevMountDevice("save", tmpfs);
        if (ret==-1) {
            printf("fsdevMountDevice() failed.\n");
            rc = ret;
        }
    }

    //At this point you can use the mounted device with standard stdio.
    //After modifying savedata, in order for the changes to take affect you must use: rc = fsdevCommitDevice("save");

    if (R_SUCCEEDED(rc)) {
        dir = opendir("save:/");//Open the "save:/" directory.
        if(dir==NULL)
        {
            printf("Failed to open dir.\n");
        }
        else
        {
            printf("Dir-listing for 'save:/':\n");
            while ((ent = readdir(dir)))
            {
                printf("d_name: %s\n", ent->d_name);
            }
            closedir(dir);
            printf("Done.\n");
        }

        //When you are done with savedata, you can use the below.
        //Any devices still mounted at app exit are automatically unmounted.
        fsdevUnmountDevice("save");
    }

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}

