#include <string.h>
#include <stdio.h>
#include <switch.h>

#include "image_bin.h"//Your own raw RGB888 1280x720 image at "data/image.bin" is required.

//See also libnx gfx.h.

int main(int argc, char **argv)
{
	u8 *framebuf;
	u32 *framebuf32;
	u32 width=0, height=0;
        u32 image_width = 1280, image_height = 720;
	u32 tmp_width, tmp_height;
	u32 x, y;
	u32 j;
	u32 pos;
	u32 pos2;
	u8 *imageptr = (u8*)image_bin;
	u32 cnt=0;

	//gfxInitResolutionDefault();//Enable max-1080p support. Remove for 720p-only resolution.
	gfxInitDefault();
	//gfxConfigureAutoResolutionDefault(true);//Set current resolution automatically depending on current/changed OperationMode. Only use this when using gfxInitResolution*().
	//Note that while the above commented code enables 1080p for docked-mode, this example only draws a 720p image, with the rest of the screen being left at white.

	//"Safe default, single Joy-Con have buttons/sticks rotated for orientation"
	hidSetControllerLayout(CONTROLLER_PLAYER_1, LAYOUT_DEFAULT);

	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		if ((hidKeysDown(CONTROLLER_PLAYER_1) | hidKeysDown(CONTROLLER_HANDHELD)) & KEY_PLUS) break;

		framebuf = gfxGetFramebuffer((u32*)&width, (u32*)&height);
		framebuf32 = (u32*)framebuf;

		if(cnt==60)
		{
			cnt=0;
		}
		else
		{
			cnt++;
		}

		tmp_width = image_width;
		tmp_height = image_height;
		if(tmp_width > width) tmp_width = width;
		if(tmp_height > height) tmp_height = height;

		memset(framebuf, 0xff, gfxGetFramebufferSize());
		for(x=0; x<image_width; x+=4)//Every 4 pixels with the below pixel-format is stored consecutively in memory.
		{
			for(y=0; y<image_height; y++)
			{
				//Each pixel is 4-bytes due to RGBA8888.
				pos = gfxGetFramebufferDisplayOffset(x, y);
				pos2 = (x + (y*image_width)) * 3;
				for(j=0; j<4; j++)framebuf32[pos+j] = RGBA8_MAXALPHA(imageptr[pos2+0+(j*3)]+(cnt*4), imageptr[pos2+1+(j*3)], imageptr[pos2+2+(j*3)]);
			}
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

