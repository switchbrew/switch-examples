#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <switch.h>

u8 *ir_buffer = NULL;

int main(int argc, char **argv)
{
	Result rc=0;
	u32 irhandle=0;
	irsImageTransferProcessorConfig config;
	irsImageTransferProcessorState state;
	size_t ir_buffer_size = 0x12c00;

	gfxInitDefault();

	//Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
	consoleInit(NULL);

	ir_buffer = (u8*)malloc(ir_buffer_size);
	if (ir_buffer==NULL)
	{
		rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
		printf("Failed to allocate memory for ir_buffer.\n");
	}
	else
	{
		memset(ir_buffer, 0, ir_buffer_size);
	}

	if (R_SUCCEEDED(rc))
	{
		rc = irsInitialize();
		printf("irsInitialize() returned 0x%x\n", rc);
	}

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

	if (R_SUCCEEDED(rc))
	{
		irsGetDefaultImageTransferProcessorConfig(&config);
		rc = irsRunImageTransferProcessor(irhandle, &config, 0x100000);
		printf("irsRunImageTransferProcessor() returned 0x%x\n", rc);
	}

	if (R_SUCCEEDED(rc))
	{
		//TODO: Why does this fail? Maybe we need to use certain hid-serv cmd(s)?
		rc = irsGetImageTransferProcessorState(irhandle, ir_buffer, ir_buffer_size, &state);
		printf("irsGetImageTransferProcessorState() returned 0x%x\n", rc);
	}

	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	irsStopImageProcessor(irhandle);
	irsExit();

	free(ir_buffer);

	gfxExit();
	return 0;
}

