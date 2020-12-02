#include <string.h>
#include <stdio.h>

#include <switch.h>

//See also libnx set.h.

int main(int argc, char **argv)
{
    u64 LanguageCode=0;
    SetLanguage Language=SetLanguage_ENUS;

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

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
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // Your code goes here

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    setExit();
    consoleExit(NULL);
    return 0;
}
