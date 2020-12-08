// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the HOME-button notification-LED, see also libnx hidsys.h.

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    Result rc=0;
    s32 i;
    s32 total_entries;
    HidsysUniquePadId unique_pad_ids[2]={0};
    HidsysNotificationLedPattern pattern;

    printf("notification-led example\n");

    rc = hidsysInitialize();
    if (R_FAILED(rc)) {
        printf("hidsysInitialize(): 0x%x\n", rc);
        printf("Init failed, press + to exit.\n");
    }
    else {
        printf("Press A to set a Breathing effect notification-LED pattern.\n");
        printf("Press B to set a Heartbeat effect notification-LED pattern.\n");
        printf("Press + to disable notification-LED and exit.\n");
    }

    // Main loop
    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) {
            // Disable notification led. Only needed with hidsysSetNotificationLedPattern, with hidsysSetNotificationLedPatternWithTimeout the LED will be automatically disabled via the timeout.
            memset(&pattern, 0, sizeof(pattern));
        }
        else if (kDown & HidNpadButton_A) {
            memset(&pattern, 0, sizeof(pattern));

            // Setup Breathing effect pattern data.
            pattern.baseMiniCycleDuration = 0x8;             // 100ms.
            pattern.totalMiniCycles = 0x2;                   // 3 mini cycles. Last one 12.5ms.
            pattern.totalFullCycles = 0x0;                   // Repeat forever.
            pattern.startIntensity = 0x2;                    // 13%.

            pattern.miniCycles[0].ledIntensity = 0xF;        // 100%.
            pattern.miniCycles[0].transitionSteps = 0xF;     // 15 steps. Transition time 1.5s.
            pattern.miniCycles[0].finalStepDuration = 0x0;   // Forced 12.5ms.
            pattern.miniCycles[1].ledIntensity = 0x2;        // 13%.
            pattern.miniCycles[1].transitionSteps = 0xF;     // 15 steps. Transition time 1.5s.
            pattern.miniCycles[1].finalStepDuration = 0x0;   // Forced 12.5ms. 
        }
        else if (kDown & HidNpadButton_B) {
            memset(&pattern, 0, sizeof(pattern));

            // Setup Heartbeat effect pattern data.
            pattern.baseMiniCycleDuration = 0x1;             // 12.5ms.
            pattern.totalMiniCycles = 0xF;                   // 16 mini cycles.
            pattern.totalFullCycles = 0x0;                   // Repeat forever.
            pattern.startIntensity = 0x0;                    // 0%.

            // First beat.
            pattern.miniCycles[0].ledIntensity = 0xF;        // 100%.
            pattern.miniCycles[0].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[0].finalStepDuration = 0x0;   // Forced 12.5ms.
            pattern.miniCycles[1].ledIntensity = 0x0;        // 0%.
            pattern.miniCycles[1].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[1].finalStepDuration = 0x0;   // Forced 12.5ms.

            // Second beat.
            pattern.miniCycles[2].ledIntensity = 0xF;
            pattern.miniCycles[2].transitionSteps = 0xF;
            pattern.miniCycles[2].finalStepDuration = 0x0;
            pattern.miniCycles[3].ledIntensity = 0x0;
            pattern.miniCycles[3].transitionSteps = 0xF;
            pattern.miniCycles[3].finalStepDuration = 0x0;

            // Led off wait time.
            for(i=2; i<15; i++) {
                pattern.miniCycles[i].ledIntensity = 0x0;        // 0%.
                pattern.miniCycles[i].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
                pattern.miniCycles[i].finalStepDuration = 0xF;   // 187.5ms.
            }
        }

        if (kDown & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_Plus)) {
            total_entries = 0;
            memset(unique_pad_ids, 0, sizeof(unique_pad_ids));

            // Get the UniquePadIds for the specified controller, which will then be used with hidsysSetNotificationLedPattern*.
            // If you want to get the UniquePadIds for all controllers, you can use hidsysGetUniquePadIds instead.
            rc = hidsysGetUniquePadsFromNpad(padIsHandheld(&pad) ? HidNpadIdType_Handheld : HidNpadIdType_No1, unique_pad_ids, 2, &total_entries);
            printf("hidsysGetUniquePadsFromNpad(): 0x%x", rc);
            if (R_SUCCEEDED(rc)) printf(", %d", total_entries);
            printf("\n");

            if (R_SUCCEEDED(rc)) {
                for(i=0; i<total_entries; i++) { // System will skip sending the subcommand to controllers where this isn't available.
                    printf("[%d] = 0x%lx ", i, unique_pad_ids[i].id);

                    // Attempt to use hidsysSetNotificationLedPatternWithTimeout first with a 2 second timeout, then fallback to hidsysSetNotificationLedPattern on failure. See hidsys.h for the requirements for using these.
                    rc = hidsysSetNotificationLedPatternWithTimeout(&pattern, unique_pad_ids[i], 2000000000ULL);
                    printf("hidsysSetNotificationLedPatternWithTimeout(): 0x%x\n", rc);
                    if (R_FAILED(rc)) {
                        rc = hidsysSetNotificationLedPattern(&pattern, unique_pad_ids[i]);
                        printf("hidsysSetNotificationLedPattern(): 0x%x\n", rc);
                    }
                }
            }
        }

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    hidsysExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
