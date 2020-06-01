#include <string.h>
#include <stdio.h>

#include <switch.h>

// This example shows how to use the SevenSixAxisSensor. Official sw usually only uses this while VrMode is active. See also the vrmode example.
// See also libnx hid.h.
// SevenSixAxisSensor combines the SixAxisSensor for the Console and Joy-Cons together.

int main(int argc, char **argv)
{
    Result rc=0;

    consoleInit(NULL);

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
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        if (kDown & KEY_A) {
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

        consoleUpdate(NULL);
    }

    rc = appletSetVrModeEnabled(false);
    printf("appletSetVrModeEnabled(false): 0x%x\n", rc);

    rc = hidStopSevenSixAxisSensor();
    printf("hidStopSevenSixAxisSensor(): 0x%x\n", rc);

    rc = hidFinalizeSevenSixAxisSensor();
    printf("hidFinalizeSevenSixAxisSensor(): 0x%x\n", rc);

    consoleExit(NULL);
    return 0;
}

