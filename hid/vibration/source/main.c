#include <string.h>
#include <stdio.h>

#include <switch.h>

//Example for HID vibration/rumble.

int main(int argc, char **argv)
{
	u32 VibrationDeviceHandles[2];
	Result rc = 0, rc2 = 0;

	HidVibrationValue VibrationValue;
	HidVibrationValue VibrationValue_stop;

	gfxInitDefault();
	consoleInit(NULL);

	printf("Press PLUS to exit.\n");

	//Two VibrationDeviceHandles are returned: first one for left-joycon, second one for right-joycon.
	rc = hidInitializeVibrationDevices(VibrationDeviceHandles, 2, CONTROLLER_PLAYER_1, LAYOUT_DEFAULT);
	printf("hidInitializeVibrationDevice() returned: 0x%x\n", rc);

	if (R_SUCCEEDED(rc)) printf("Hold R to vibrate, and press A/B/X/Y to adjust values.\n");

	VibrationValue.amp_low   = 0.2f;
	VibrationValue.freq_low  = 10.0f;
	VibrationValue.amp_high  = 0.2f;
	VibrationValue.freq_high = 10.0f;

	memset(&VibrationValue_stop, 0, sizeof(HidVibrationValue));
	// Switch like stop behavior with muted band channels and frequencies set to default.
	VibrationValue_stop.freq_low  = 160.0f
	VibrationValue_stop.freq_high = 320.0f

	// Main loop
	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		u32 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
		u32 kUp = hidKeysUp(CONTROLLER_P1_AUTO);

		if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

		if (R_SUCCEEDED(rc) && (kHeld & KEY_R))
		{
			//Calling hidSendVibrationValue is really only needed when sending a new VibrationValue.

			//left-joycon
			rc2 = hidSendVibrationValue(&VibrationDeviceHandles[0], &VibrationValue);
			if (R_FAILED(rc2)) printf("hidSendVibrationValue() returned: 0x%x\n", rc2);

			//right-joycon
			rc2 = hidSendVibrationValue(&VibrationDeviceHandles[1], &VibrationValue);
			if (R_FAILED(rc2)) printf("hidSendVibrationValue() returned: 0x%x\n", rc2);

			if (kDown & KEY_A) VibrationValue.amp_low   += 0.1f;
			if (kDown & KEY_B) VibrationValue.freq_low  += 5.0f;
			if (kDown & KEY_X) VibrationValue.amp_high  += 0.1f;
			if (kDown & KEY_Y) VibrationValue.freq_high += 12.0f;
		}
		else if(kUp & KEY_R)//Stop vibration.
		{
			rc2 = hidSendVibrationValue(&VibrationDeviceHandles[0], &VibrationValue_stop);
			if (R_FAILED(rc2)) printf("hidSendVibrationValue() for stop returned: 0x%x\n", rc2);

			rc2 = hidSendVibrationValue(&VibrationDeviceHandles[1], &VibrationValue_stop);
			if (R_FAILED(rc2)) printf("hidSendVibrationValue() for stop returned: 0x%x\n", rc2);
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

