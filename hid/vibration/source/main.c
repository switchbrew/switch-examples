#include <string.h>
#include <stdio.h>

#include <switch.h>

//Example for HID vibration/rumble.
//For vibration to work you may have to do the following first: enter System-Settings->Controllers, then turn the vibration config to OFF (if it's already ON), then ON.

int main(int argc, char **argv)
{
    u32 VibrationDeviceHandles[2][2];
    Result rc = 0, rc2 = 0;
    u32 target_device=0;

    HidVibrationValue VibrationValue;
    HidVibrationValue VibrationValue_stop;
    HidVibrationValue VibrationValues[2];

    consoleInit(NULL);

    printf("Press PLUS to exit.\n");

    //Two VibrationDeviceHandles are returned: first one for left-joycon, second one for right-joycon.
    //Change the total_handles param to 1, and update the hidSendVibrationValues calls, if you only want 1 VibrationDeviceHandle.
    rc = hidInitializeVibrationDevices(VibrationDeviceHandles[0], 2, CONTROLLER_HANDHELD, TYPE_HANDHELD | TYPE_JOYCON_PAIR);

    //Setup VibrationDeviceHandles for CONTROLLER_PLAYER_1 too, since we want to support both CONTROLLER_HANDHELD and CONTROLLER_PLAYER_1.
    if (R_SUCCEEDED(rc)) rc = hidInitializeVibrationDevices(VibrationDeviceHandles[1], 2, CONTROLLER_PLAYER_1, TYPE_HANDHELD | TYPE_JOYCON_PAIR);
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
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        u64 kUp = hidKeysUp(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        //Select which devices to vibrate.
        target_device = 0;
        if (!hidGetHandheldMode())
            target_device = 1;

        if (R_SUCCEEDED(rc) && (kHeld & KEY_R))
        {
            //Calling hidSendVibrationValue/hidSendVibrationValues is really only needed when sending new VibrationValue(s).
            //If you just want to vibrate 1 device, you can also use hidSendVibrationValue.

            memcpy(&VibrationValues[0], &VibrationValue, sizeof(HidVibrationValue));
            memcpy(&VibrationValues[1], &VibrationValue, sizeof(HidVibrationValue));

            rc2 = hidSendVibrationValues(VibrationDeviceHandles[target_device], VibrationValues, 2);
            if (R_FAILED(rc2)) printf("hidSendVibrationValues() returned: 0x%x\n", rc2);

            if (kDown & KEY_A) VibrationValue.amp_low   += 0.1f;
            if (kDown & KEY_B) VibrationValue.freq_low  += 5.0f;
            if (kDown & KEY_X) VibrationValue.amp_high  += 0.1f;
            if (kDown & KEY_Y) VibrationValue.freq_high += 12.0f;
        }
        else if(kUp & KEY_R)//Stop vibration for all devices.
        {
            memcpy(&VibrationValues[0], &VibrationValue_stop, sizeof(HidVibrationValue));
            memcpy(&VibrationValues[1], &VibrationValue_stop, sizeof(HidVibrationValue));

            rc2 = hidSendVibrationValues(VibrationDeviceHandles[target_device], VibrationValues, 2);
            if (R_FAILED(rc2)) printf("hidSendVibrationValues() for stop returned: 0x%x\n", rc2);

            //Could also do this with 1 hidSendVibrationValues() call + a larger VibrationValues array.
            rc2 = hidSendVibrationValues(VibrationDeviceHandles[1-target_device], VibrationValues, 2);
            if (R_FAILED(rc2)) printf("hidSendVibrationValues() for stop other device returned: 0x%x\n", rc2);
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
