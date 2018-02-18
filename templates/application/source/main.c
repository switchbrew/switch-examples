#include <string.h>
#include <stdio.h>

#include <switch.h>

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(NULL);

	printf("Hello World!");

	// Main loop
	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		// Your code goes here

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

