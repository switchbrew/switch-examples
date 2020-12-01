#include <string.h>
#include <stdio.h>

#include <switch.h>

//See also libnx hid.h.

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    // It's necessary to initialize these separately as they all have different handle values
    HidSixAxisSensorHandle handles[4];
    hidGetSixAxisSensorHandles(&handles[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidGetSixAxisSensorHandles(&handles[1], 1, HidNpadIdType_No1,      HidNpadStyleTag_NpadFullKey);
    hidGetSixAxisSensorHandles(&handles[2], 2, HidNpadIdType_No1,      HidNpadStyleTag_NpadJoyDual);
    hidStartSixAxisSensor(handles[0]);
    hidStartSixAxisSensor(handles[1]);
    hidStartSixAxisSensor(handles[2]);
    hidStartSixAxisSensor(handles[3]);

    printf("\x1b[1;1HPress PLUS to exit.");
    printf("\x1b[2;1HSixAxis Sensor readings:");

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        // Read from the correct sixaxis handle depending on the current input style
        HidSixAxisSensorState sixaxis = {0};
        u64 style_set = padGetStyleSet(&pad);
        if (style_set & HidNpadStyleTag_NpadHandheld)
            hidGetSixAxisSensorStates(handles[0], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadFullKey)
            hidGetSixAxisSensorStates(handles[1], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadJoyDual) {
            // For JoyDual, read from either the Left or Right Joy-Con depending on which is/are connected
            u64 attrib = padGetAttributes(&pad);
            if (attrib & HidNpadAttribute_IsLeftConnected)
                hidGetSixAxisSensorStates(handles[2], &sixaxis, 1);
            else if (attrib & HidNpadAttribute_IsRightConnected)
                hidGetSixAxisSensorStates(handles[3], &sixaxis, 1);
        }

        printf("\x1b[3;1H");

        printf("Acceleration:     x=% .4f, y=% .4f, z=% .4f\n", sixaxis.acceleration.x, sixaxis.acceleration.y, sixaxis.acceleration.z);
        printf("Angular velocity: x=% .4f, y=% .4f, z=% .4f\n", sixaxis.angular_velocity.x, sixaxis.angular_velocity.y, sixaxis.angular_velocity.z);
        printf("Angle:            x=% .4f, y=% .4f, z=% .4f\n", sixaxis.angle.x, sixaxis.angle.y, sixaxis.angle.z);
        printf("Direction matrix:\n"
               "                  [ % .4f,   % .4f,   % .4f ]\n"
               "                  [ % .4f,   % .4f,   % .4f ]\n"
               "                  [ % .4f,   % .4f,   % .4f ]\n",
            sixaxis.direction.direction[0][0], sixaxis.direction.direction[1][0], sixaxis.direction.direction[2][0],
            sixaxis.direction.direction[0][1], sixaxis.direction.direction[1][1], sixaxis.direction.direction[2][1],
            sixaxis.direction.direction[0][2], sixaxis.direction.direction[1][2], sixaxis.direction.direction[2][2]);

        consoleUpdate(NULL);
    }

    hidStopSixAxisSensor(handles[0]);
    hidStopSixAxisSensor(handles[1]);
    hidStopSixAxisSensor(handles[2]);
    hidStopSixAxisSensor(handles[3]);

    consoleExit(NULL);
    return 0;
}
