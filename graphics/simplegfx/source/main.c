#include <string.h>
#include <stdio.h>

#include <switch.h>

int main(int argc, char **argv)
{
	u32 cnt=60*5;

	gfxInitDefault();

	while(cnt)//Unknown how to properly handle exiting this loop.
	{
		//Currently unknown how to write to {framebuffer}.

		gfxSwapBuffers();
		gfxWaitForVsync();
		cnt--;
	}

	gfxExit();
	return 0;
}

