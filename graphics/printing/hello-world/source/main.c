#include <string.h>
#include <stdio.h>

#include <switch.h>

int main(int argc, char **argv)
{
	gfxInitDefault();

	//Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
	consoleInit(NULL);

	//Move the cursor to row 16 and column 20 and then prints "Hello World!"
	//To move the cursor you have to print "\x1b[r;cH", where r and c are respectively
	//the row and column where you want your cursor to move
	printf("\x1b[16;20HHello World!");

	//"Safe default, single Joy-Con have buttons/sticks rotated for orientation"
	hidSetControllerLayout(CONTROLLER_PLAYER_1, LAYOUT_DEFAULT);

	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		if ((hidKeysDown(CONTROLLER_PLAYER_1) | hidKeysDown(CONTROLLER_HANDHELD)) & KEY_PLUS) break;

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

