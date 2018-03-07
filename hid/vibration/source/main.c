#include <string.h>
#include <stdio.h>

#include <switch.h>

//Example for HID vibration/rumble.

int main(int argc, char **argv)
{
	u32 VibrationDeviceHandle=0;
	Result rc = 0, rc2 = 0;

	HidVibrationValue VibrationValue;
	HidVibrationValue VibrationValue_stop;

	gfxInitDefault();
	consoleInit(NULL);

	printf("Press PLUS to exit.\n");

	rc = hidInitializeVibrationDevice(&VibrationDeviceHandle, CONTROLLER_PLAYER_1, LAYOUT_DEFAULT);
	printf("hidInitializeVibrationDevice() returned: 0x%x\n", rc);

	if (R_SUCCEEDED(rc)) printf("Hold R to vibrate, and press A/B/X/Y to adjust values.\n");

	VibrationValue.values[0] = 10.0f;
	VibrationValue.values[1] = 10.0f;
	VibrationValue.values[2] = 10.0f;
	VibrationValue.values[3] = 10.0f;

	memset(&VibrationValue_stop, 0, sizeof(HidVibrationValue));

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
			rc2 = hidSendVibrationValue(&VibrationDeviceHandle, &VibrationValue);//This is really only needed when sending a new VibrationValue.
			if (R_FAILED(rc2)) printf("hidSendVibrationValue() returned: 0x%x\n", rc2);

			if (kDown & KEY_A) VibrationValue.values[0]+= 1.0f;
			if (kDown & KEY_B) VibrationValue.values[1]+= 5.0f;
			if (kDown & KEY_X) VibrationValue.values[2]+= 10.0f;
			if (kDown & KEY_Y) VibrationValue.values[3]+= 12.0f;
		}
		else if(kUp & KEY_R)//Stop vibration.
		{
			rc2 = hidSendVibrationValue(&VibrationDeviceHandle, &VibrationValue_stop);
			if (R_FAILED(rc2)) printf("hidSendVibrationValue() for stop returned: 0x%x\n", rc2);
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

