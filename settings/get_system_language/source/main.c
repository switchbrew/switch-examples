#include <string.h>
#include <stdio.h>

#include <switch.h>

//See also libnx set.h.

int main(int argc, char **argv)
{
    u64 LanguageCode=0;
    s32 Language=0;

    consoleInit(NULL);

    Result rc = setInitialize();
    if (R_FAILED(rc)) printf("setInitialize() failed: 0x%x.\n", rc);

    if (R_SUCCEEDED(rc))
    {
        //Get system language.
        rc = setGetSystemLanguage(&LanguageCode);
        if (R_FAILED(rc)) printf("setGetSystemLanguage() failed: 0x%x.\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        printf("setGetSystemLanguage() LanguageCode: %s\n", (char*)&LanguageCode);

        // Convert LanguageCode to Language. Use this if you need IDs, not strings.
        rc = setMakeLanguage(LanguageCode, &Language);

        if (R_FAILED(rc)) printf("setMakeLanguage() failed: 0x%x.\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        printf("Language: %d\n", Language);

        // Converts the input Language to LanguageCode.
        rc = setMakeLanguageCode(SetLanguage_JA, &LanguageCode);

        if (R_FAILED(rc)) printf("setMakeLanguageCode() failed: 0x%x.\n", rc);
    }

    if (R_SUCCEEDED(rc)) printf("New LanguageCode: %s\n", (char*)&LanguageCode);

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

    setExit();
    consoleExit(NULL);
    return 0;
}
