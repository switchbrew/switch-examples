#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <switch.h>

//This example shows how to access savedata for (official) applications/games.

Result get_save(u64 *application_id, AccountUid *uid) {
    Result rc=0;
    FsSaveDataInfoReader reader;
    s64 total_entries=0;
    FsSaveDataInfo info;
    bool found=0;

    rc = fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);//See libnx fs.h.
    if (R_FAILED(rc)) {
        printf("fsOpenSaveDataInfoReader() failed: 0x%x\n", rc);
        return rc;
    }

    //Find the first savedata with FsSaveDataType_SaveData.
    while(1) {
        rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);//See libnx fs.h.
        if (R_FAILED(rc) || total_entries==0) break;

        if (info.save_data_type == FsSaveDataType_Account) {//Filter by FsSaveDataType_Account, however note that FsSaveDataSpaceId_User can have non-FsSaveDataType_Account.
            *application_id = info.application_id;
            *uid = info.uid;
            found = 1;
            break;
        }
    }

    fsSaveDataInfoReaderClose(&reader);

    if (R_SUCCEEDED(rc) && !found) return MAKERESULT(Module_Libnx, LibnxError_NotFound);

    return rc;
}

int main(int argc, char **argv)
{
    Result rc=0;

    DIR* dir;
    struct dirent* ent;

    AccountUid uid={0};
    u64 application_id=0x01007ef00011e000;//ApplicationId of the save to mount, in this case BOTW.

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    //Get the userID for save mounting. To mount common savedata, use an all-zero userID.

    //Try to find savedata to use with get_save() first, otherwise fallback to the above hard-coded TID + the userID from accountGetPreselectedUser(). Note that you can use either method.
    //See the account example for getting account info for an userID.
    //See also the app_controldata example for getting info for an application_id.
    if (R_FAILED(get_save(&application_id, &uid))) {
        rc = accountInitialize(AccountServiceType_Application);
        if (R_FAILED(rc)) {
            printf("accountInitialize() failed: 0x%x\n", rc);
        }

        if (R_SUCCEEDED(rc)) {
            rc = accountGetPreselectedUser(&uid);
            accountExit();

            if (R_FAILED(rc)) {
                printf("accountGetPreselectedUser() failed: 0x%x\n", rc);
            }
        }
    }

    if (R_SUCCEEDED(rc)) {
        printf("Using application_id=0x%016lx uid: 0x%lx 0x%lx\n", application_id, uid.uid[1], uid.uid[0]);
    }

    //You can use any device-name. If you want multiple saves mounted at the same time, you must use different device-names for each one.
    if (R_SUCCEEDED(rc)) {
        rc = fsdevMountSaveData("save", application_id, uid);//See also libnx fs.h/fs_dev.h
        if (R_FAILED(rc)) {
            printf("fsdevMountSaveData() failed: 0x%x\n", rc);
        }
    }

    //At this point you can use the mounted device with standard stdio.
    //After modifying savedata, in order for the changes to take affect you must use: rc = fsdevCommitDevice("save");
    //See also libnx fs_dev.h for fsdevCommitDevice.

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
