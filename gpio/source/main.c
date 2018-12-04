// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Include the main libnx system header, for Switch development
#include <switch.h>
// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses the gpio, as a way to detect when the volume buttons have been pressed

    consoleInit(NULL);
    gpioInitialize();
    GpioPadSession vol_up_btn, vol_down_btn;

    // Not all the GpioPadName are declared, so you have to add her hex value and do a casting
    // gpioOpenSession(&joycon_L_attach, (GpioPadName)0x0c);
    // https://switchbrew.org/wiki/Bus_services#Known_Devices

    gpioOpenSession(&vol_up_btn, GpioPadName_ButtonVolUp);
    gpioOpenSession(&vol_down_btn, GpioPadName_ButtonVolDown);

    // Set direction input
    gpioPadSetDirection(&vol_up_btn, GpioDirection_Input);
    gpioPadSetDirection(&vol_down_btn, GpioDirection_Input);

    GpioValue val1, val2;

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

        gpioPadGetValue(&vol_up_btn, &val1);
        gpioPadGetValue(&vol_down_btn, &val2);

        if (val1 == GpioValue_Low) {
            consoleClear();
            printf("Volume up button pressed\n");
        }

        if (val2 == GpioValue_Low) {
            consoleClear();
            printf("Volume down button pressed\n");
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }
    // Deinitialize and clean up resources used by the console (important!)
    gpioPadClose(&vol_up_btn);
    gpioPadClose(&vol_down_btn);
    gpioExit();
    consoleExit(NULL);
    return 0;
}
