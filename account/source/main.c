#include <string.h>
#include <stdio.h>

#include <switch.h>

//This example shows how to get info for the preselected user account. See libnx acc.h.

int main(int argc, char **argv)
{
    Result rc=0;

    AccountUid userID={0};
    AccountProfile profile;
    AccountUserData userdata;
    AccountProfileBase profilebase;

    char nickname[0x21];

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    memset(&userdata, 0, sizeof(userdata));
    memset(&profilebase, 0, sizeof(profilebase));

    rc = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(rc)) {
        printf("accountInitialize() failed: 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        rc = accountGetPreselectedUser(&userID);

        if (R_FAILED(rc)) {
            printf("accountGetPreselectedUser() failed: 0x%x\n", rc);
        }

        if (R_SUCCEEDED(rc)) {
            printf("Current userID: 0x%lx 0x%lx\n", userID.uid[1], userID.uid[0]);

            rc = accountGetProfile(&profile, userID);

            if (R_FAILED(rc)) {
                printf("accountGetProfile() failed: 0x%x\n", rc);
            }
        }

        if (R_SUCCEEDED(rc)) {
            rc = accountProfileGet(&profile, &userdata, &profilebase);//userdata is otional, see libnx acc.h.

            if (R_FAILED(rc)) {
                printf("accountProfileGet() failed: 0x%x\n", rc);
            }

            if (R_SUCCEEDED(rc)) {
                memset(nickname,  0, sizeof(nickname));
                strncpy(nickname, profilebase.nickname, sizeof(nickname)-1);//Copy the nickname elsewhere to make sure it's NUL-terminated.

                printf("Nickname: %s\n", nickname);//Note that the print-console doesn't support UTF-8. The nickname is UTF-8, so this will only display properly if there isn't any non-ASCII characters. To display it properly, a print method which supports UTF-8 should be used instead.

                //You can also use accountProfileGetImageSize()/accountProfileLoadImage() to load the icon for use with a JPEG library, for displaying the user profile icon. See libnx acc.h.
            }

            accountProfileClose(&profile);
        }

        accountExit();
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
