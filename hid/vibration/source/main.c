#include <string.h>
#include <stdio.h>

#include <switch.h>

//Example for HID vibration/rumble.
//TODO: Why doesn't this vibrate, even though no errors are returned?

int main(int argc, char **argv)
{
	u32 VibrationDeviceHandle=0;
	Result rc = 0, rc2 = 0;

	HidVibrationValue VibrationValue;

	gfxInitDefault();
	consoleInit(NULL);

	printf("Press PLUS to exit.\n");

	rc = hidInitializeVibrationDevice(&VibrationDeviceHandle, CONTROLLER_PLAYER_1, LAYOUT_DEFAULT);//3-10 works
	printf("hidInitializeVibrationDevice() returned: 0x%x\n", rc);

	if (R_SUCCEEDED(rc)) printf("Press A to vibrate.\n");

	VibrationValue.values[0] = -0.5f;
	VibrationValue.values[1] = -0.5f;
	VibrationValue.values[2] = -0.5f;
	VibrationValue.values[3] = -0.5f;

	// Main loop
	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

		if (R_SUCCEEDED(rc) && (kDown & KEY_A))
		{
			rc2 = hidSendVibrationValue(&VibrationDeviceHandle, &VibrationValue);
			printf("hidSendVibrationValue() returned: 0x%x\n", rc2);
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

