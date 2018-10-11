#include <string.h>
#include <stdio.h>

#include <switch.h>

//See also libnx hid.h.

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // It's necessary to initialize these separately as they all have different handle values
    u32 handles[4];
    hidGetSixAxisSensorHandles(&handles[0], 2, CONTROLLER_PLAYER_1, TYPE_JOYCON_PAIR);
    hidGetSixAxisSensorHandles(&handles[2], 1, CONTROLLER_PLAYER_1, TYPE_PROCONTROLLER);
    hidGetSixAxisSensorHandles(&handles[3], 1, CONTROLLER_HANDHELD, TYPE_HANDHELD);
    hidStartSixAxisSensor(handles[0]);
    hidStartSixAxisSensor(handles[1]);
    hidStartSixAxisSensor(handles[2]);
    hidStartSixAxisSensor(handles[3]);

    printf("\x1b[1;1HPress PLUS to exit.");
    printf("\x1b[2;1HSixAxis Sensor readings:");

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        SixAxisSensorValues sixaxis;
        // You can read back up to 17 successive values at once
        hidSixAxisSensorValuesRead(&sixaxis, CONTROLLER_P1_AUTO, 1);

        printf("\x1b[3;1H");

        printf("Accelerometer:    x=% .4f, y=% .4f, z=% .4f\n", sixaxis.accelerometer.x, sixaxis.accelerometer.y, sixaxis.accelerometer.z);
        printf("Gyroscope:        x=% .4f, y=% .4f, z=% .4f\n", sixaxis.gyroscope.x, sixaxis.gyroscope.y, sixaxis.gyroscope.z);
        // It's currently unknown what this corresponds to other than some rotational value
        printf("Unknown rotation: x=% .4f, y=% .4f, z=% .4f\n", sixaxis.unk.x, sixaxis.unk.y, sixaxis.unk.z);
        printf("Orientation matrix:\n"
               "                  [ % .4f,   % .4f,   % .4f ]\n"
               "                  [ % .4f,   % .4f,   % .4f ]\n"
               "                  [ % .4f,   % .4f,   % .4f ]\n",
            sixaxis.orientation[0].x, sixaxis.orientation[0].y, sixaxis.orientation[0].z,
            sixaxis.orientation[1].x, sixaxis.orientation[1].y, sixaxis.orientation[1].z,
            sixaxis.orientation[2].x, sixaxis.orientation[2].y, sixaxis.orientation[2].z);

        consoleUpdate(NULL);
    }

    hidStopSixAxisSensor(handles[0]);
    hidStopSixAxisSensor(handles[1]);
    hidStopSixAxisSensor(handles[2]);
    hidStopSixAxisSensor(handles[3]);

    consoleExit(NULL);
    return 0;
}

