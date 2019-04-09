// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// See also libnx swkbd.h.

// This example shows how to use the software keyboard (swkbd) LibraryApplet.

// TextCheck callback, this can be removed when not using TextCheck.
SwkbdTextCheckResult validate_text(char* tmp_string, size_t tmp_string_size) {
    if (strcmp(tmp_string, "bad")==0) {
        strncpy(tmp_string, "Bad string.", tmp_string_size);
        return SwkbdTextCheckResult_Bad;
    }

    return SwkbdTextCheckResult_OK;
}

// The rest of these callbacks are for swkbd-inline.
void finishinit_cb(void) {
    printf("reply: FinishedInitialize\n");
}

// String changed callback.
void strchange_cb(const char* str, SwkbdChangedStringArg* arg) {
    printf("reply: ChangedString. str = %s, arg->stringLen = 0x%x, arg->dicStartCursorPos = 0x%x, arg->dicEndCursorPos = 0x%x, arg->arg->cursorPos = 0x%x\n", str, arg->stringLen, arg->dicStartCursorPos, arg->dicEndCursorPos, arg->cursorPos);
}

// Moved cursor callback.
void movedcursor_cb(const char* str, SwkbdMovedCursorArg* arg) {
    printf("reply: MovedCursor. str = %s, arg->stringLen = 0x%x, arg->cursorPos = 0x%x\n", str, arg->stringLen, arg->cursorPos);
}

// Text submitted callback, this is used to get the output text from submit.
void decidedenter_cb(const char* str, SwkbdDecidedEnterArg* arg) {
    printf("reply: DecidedEnter. str = %s, arg->stringLen = 0x%x\n", str, arg->stringLen);
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx gfx API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("swkbd example\n");

    consoleUpdate(NULL);

    SwkbdConfig kbd;
    char tmpoutstr[16] = {0};
    rc = swkbdCreate(&kbd, 0);
    printf("swkbdCreate(): 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        // Select a Preset to use, if any.
        swkbdConfigMakePresetDefault(&kbd);
        //swkbdConfigMakePresetPassword(&kbd);
        //swkbdConfigMakePresetUserName(&kbd);
        //swkbdConfigMakePresetDownloadCode(&kbd);

        // Optional, set any text if you want (see swkbd.h).
        //swkbdConfigSetOkButtonText(&kbd, "Submit");
        //swkbdConfigSetLeftOptionalSymbolKey(&kbd, "a");
        //swkbdConfigSetRightOptionalSymbolKey(&kbd, "b");
        //swkbdConfigSetHeaderText(&kbd, "Header");
        //swkbdConfigSetSubText(&kbd, "Sub");
        //swkbdConfigSetGuideText(&kbd, "Guide");

        swkbdConfigSetTextCheckCallback(&kbd, validate_text);//Optional, can be removed if not using TextCheck.

        // Set the initial string if you want.
        //swkbdConfigSetInitialText(&kbd, "Initial");

        // You can also use swkbdConfigSet*() funcs if you want.

        printf("Running swkbdShow...\n");
        rc = swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
        printf("swkbdShow(): 0x%x\n", rc);

        if (R_SUCCEEDED(rc)) {
            printf("out str: %s\n", tmpoutstr);
        }
        swkbdClose(&kbd);
    }

    // The rest of this example shows how to use swkbd-inline, which can be ignored if you're not using swkbd-inline. See libnx swkbd.h.

    SwkbdInline kbdinline;
    rc = swkbdInlineCreate(&kbdinline);
    printf("swkbdInlineCreate(): 0x%x\n", rc);

    // Set the callbacks.
    swkbdInlineSetFinishedInitializeCallback(&kbdinline, finishinit_cb);
    swkbdInlineSetChangedStringCallback(&kbdinline, strchange_cb);
    swkbdInlineSetMovedCursorCallback(&kbdinline, movedcursor_cb);
    swkbdInlineSetDecidedEnterCallback(&kbdinline, decidedenter_cb);

    // mode has to be set to this for the applet itself to handle gfx-display.
    kbdinline.calcArg.initArg.mode = SwkbdInlineMode_AppletDisplay;

    // Launch the applet.
    if (R_SUCCEEDED(rc)) {
        rc = swkbdInlineLaunch(&kbdinline);
        printf("swkbdInlineLaunch(): 0x%x\n", rc);
    }

    // Optionally set swkbd-inline state, this can also be done after the applet appears. swkbdInlineUpdate() must be called for changes to take affect.
    //swkbdInlineSetInputText(&kbdinline, "test");
    //swkbdInlineUpdate(&kbdinline, NULL);

    // Make the applet appear, can be used whenever.
    SwkbdAppearArg appearArg;
    swkbdInlineMakeAppearArg(&appearArg, SwkbdType_Normal);
    // You can optionally set appearArg text / fields here.
    swkbdInlineAppearArgSetOkButtonText(&appearArg, "Submit");
    appearArg.dicFlag = 1;
    appearArg.returnButtonFlag = 1;
    swkbdInlineAppear(&kbdinline, &appearArg);

    if (R_SUCCEEDED(rc)) {
        rc = swkbdInlineUpdate(&kbdinline, NULL);
        printf("swkbdInlineUpdate(): 0x%x\n", rc);
    }

    printf("Press + to exit.\n");

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        // Your code goes here

        if (R_SUCCEEDED(rc)) {
            if (kDown & KEY_ZR) swkbdInlineDisappear(&kbdinline); //Optional, you can have swkbd (dis)appear whenever.
            if (kDown & KEY_Y) swkbdInlineAppear(&kbdinline, &appearArg); // If you use swkbdInlineAppear again after text was submitted, you may want to use swkbdInlineSetInputText since the current-text will be the same as when it was submitted otherwise.

            rc = swkbdInlineUpdate(&kbdinline, NULL); // Handles updating SwkbdInline state, this should be called periodically.
            if (R_FAILED(rc)) printf("swkbdInlineUpdate(): 0x%x\n", rc);
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Normally the applet will keep running until swkbdInlineClose is used.
    rc = swkbdInlineClose(&kbdinline);
    printf("swkbdInlineClose(): 0x%x\n", rc);

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
