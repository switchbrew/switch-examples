// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// Example for HID vibration/rumble.

// Main program entrypoint
int main(int argc, char* argv[])
{
    HidVibrationDeviceHandle VibrationDeviceHandles[2][2];
    Result rc = 0, rc2 = 0;
    u32 target_device=0;

    HidVibrationValue VibrationValue;
    HidVibrationValue VibrationValue_stop;
    HidVibrationValue VibrationValues[2];

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("Press PLUS to exit.\n");

    // Two VibrationDeviceHandles are returned: first one for left-joycon, second one for right-joycon.
    // Change the total_handles param to 1, and update the hidSendVibrationValues calls, if you only want 1 VibrationDeviceHandle.
    rc = hidInitializeVibrationDevices(VibrationDeviceHandles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);

    // Setup VibrationDeviceHandles for HidNpadIdType_No1 too, since we want to support both HidNpadIdType_Handheld and HidNpadIdType_No1.
    if (R_SUCCEEDED(rc)) rc = hidInitializeVibrationDevices(VibrationDeviceHandles[1], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);
    printf("hidInitializeVibrationDevices() returned: 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) printf("Hold R to vibrate, and press A/B/X/Y while holding R to adjust values.\n");

    VibrationValue.amp_low   = 0.2f;
    VibrationValue.freq_low  = 10.0f;
    VibrationValue.amp_high  = 0.2f;
    VibrationValue.freq_high = 10.0f;

    memset(VibrationValues, 0, sizeof(VibrationValues));

    memset(&VibrationValue_stop, 0, sizeof(HidVibrationValue));
    // Switch like stop behavior with muted band channels and frequencies set to default.
    VibrationValue_stop.freq_low  = 160.0f;
    VibrationValue_stop.freq_high = 320.0f;

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);
        u64 kUp = padGetButtonsUp(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        //Select which devices to vibrate.
        target_device = padIsHandheld(&pad) ? 0 : 1;

        if (R_SUCCEEDED(rc) && (kHeld & HidNpadButton_R))
        {
            //Calling hidSendVibrationValue/hidSendVibrationValues is really only needed when sending new VibrationValue(s).
            //If you just want to vibrate 1 device, you can also use hidSendVibrationValue.

            memcpy(&VibrationValues[0], &VibrationValue, sizeof(HidVibrationValue));
            memcpy(&VibrationValues[1], &VibrationValue, sizeof(HidVibrationValue));

            rc2 = hidSendVibrationValues(VibrationDeviceHandles[target_device], VibrationValues, 2);
            if (R_FAILED(rc2)) printf("hidSendVibrationValues() returned: 0x%x\n", rc2);

            if (kDown & HidNpadButton_A) VibrationValue.amp_low   += 0.1f;
            if (kDown & HidNpadButton_B) VibrationValue.freq_low  += 5.0f;
            if (kDown & HidNpadButton_X) VibrationValue.amp_high  += 0.1f;
            if (kDown & HidNpadButton_Y) VibrationValue.freq_high += 12.0f;
        }
        else if(kUp & HidNpadButton_R)//Stop vibration for all devices.
        {
            memcpy(&VibrationValues[0], &VibrationValue_stop, sizeof(HidVibrationValue));
            memcpy(&VibrationValues[1], &VibrationValue_stop, sizeof(HidVibrationValue));

            rc2 = hidSendVibrationValues(VibrationDeviceHandles[target_device], VibrationValues, 2);
            if (R_FAILED(rc2)) printf("hidSendVibrationValues() for stop returned: 0x%x\n", rc2);

            //Could also do this with 1 hidSendVibrationValues() call + a larger VibrationValues array.
            rc2 = hidSendVibrationValues(VibrationDeviceHandles[1-target_device], VibrationValues, 2);
            if (R_FAILED(rc2)) printf("hidSendVibrationValues() for stop other device returned: 0x%x\n", rc2);
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
