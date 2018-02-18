#include <string.h>
#include <stdio.h>

#include <switch.h>

//TODO: Implement image-transfer in libnx and in this example.

int main(int argc, char **argv)
{
	Result rc=0;
	u32 irhandle=0;

	gfxInitDefault();

	//Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
	consoleInit(NULL);

	rc = irsInitialize();
	printf("irsInitialize() returned 0x%x\n", rc);

	if (R_SUCCEEDED(rc))
	{
		rc = irsActivateIrsensor(1);
		printf("irsActivateIrsensor() returned 0x%x\n", rc);
	}

	if (R_SUCCEEDED(rc))
	{
		rc = irsGetIrCameraHandle(&irhandle, CONTROLLER_PLAYER_1);
		printf("irsGetIrCameraHandle() returned 0x%x\n", rc);
	}

	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS) break; // break in order to return to hbmenu

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	irsExit();

	gfxExit();
	return 0;
}

