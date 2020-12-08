// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the SevenSixAxisSensor. Official sw usually only uses this while VrMode is active. See also the vrmode example.
// See also libnx hid.h.
// SevenSixAxisSensor combines the SixAxisSensor for the Console and Joy-Cons together.

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

    printf("Press A to get state.\n");
    printf("Press + to exit.\n");

    rc = hidInitializeSevenSixAxisSensor();
    printf("hidInitializeSevenSixAxisSensor(): 0x%x\n", rc);

    rc = appletSetVrModeEnabled(true);
    printf("appletSetVrModeEnabled(true): 0x%x\n", rc);

    rc = hidResetSevenSixAxisSensorTimestamp(); // Official sw uses this right before hidStartSevenSixAxisSensor, each time.
    printf("hidResetSevenSixAxisSensorTimestamp(): 0x%x\n", rc);

    rc = hidStartSevenSixAxisSensor();
    printf("hidStartSevenSixAxisSensor(): 0x%x\n", rc);

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        if (kDown & HidNpadButton_A) {
            HidSevenSixAxisSensorState states[1]={0};
            size_t total_out=0;
            rc = hidGetSevenSixAxisSensorStates(states, 1, &total_out);
            printf("hidGetSevenSixAxisSensorStates(): 0x%x, %lx\n", rc, total_out);

            if (R_SUCCEEDED(rc) && total_out) {
                printf("unk_x18: ");
                for (u32 i=0; i<sizeof(states[0].unk_x18)/sizeof(float); i++) printf("%f ", states[0].unk_x18[i]);
                printf("\n");
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    rc = appletSetVrModeEnabled(false);
    printf("appletSetVrModeEnabled(false): 0x%x\n", rc);

    rc = hidStopSevenSixAxisSensor();
    printf("hidStopSevenSixAxisSensor(): 0x%x\n", rc);

    rc = hidFinalizeSevenSixAxisSensor();
    printf("hidFinalizeSevenSixAxisSensor(): 0x%x\n", rc);

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}

