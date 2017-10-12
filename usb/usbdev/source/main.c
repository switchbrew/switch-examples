#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <switch.h>

//Example for usbdev, see libnx usb_dev.h. Switch-as-device<>host USB comms for serial.

Result usbdev_test(u8 *tmpbuf)
{
	u32 somepos;
	size_t transfer_sizes[10];
	u32 *tmpbuf32 = (u32*)tmpbuf;

	//Note that usbDevRead/usbDevWrite return the actual-transfer-size.

	memset(transfer_sizes, 0, sizeof(transfer_sizes));

	for(somepos=0; somepos<0x101; somepos++)
	{
		memset(tmpbuf, 0, 0x200);
		char *strptr = "Hello World!\n";

		if(somepos==0 || somepos==0x101)
		{
			transfer_sizes[0] = usbDevWrite(strptr, strlen(strptr));
		}
		else
		{
			tmpbuf[0] = somepos-1;
			usbDevWrite(tmpbuf, 1);
		}
	}

	memset(tmpbuf, 0, 0x210);
	strncpy((char*)tmpbuf, "Hey: ", 4);

	transfer_sizes[1] = usbDevRead(&tmpbuf[4], 0x200);
	tmpbuf32[(4+0x200+0)>>2] = transfer_sizes[0];
	tmpbuf32[(4+0x200+4)>>2] = transfer_sizes[1];

	usbDevWrite(tmpbuf, 4+0x200+8);

	for(somepos=0; somepos<(0x10000>>2); somepos++) {
		tmpbuf32[somepos] = 0x41414141 + somepos;
	}

	transfer_sizes[0] = usbDevWrite(tmpbuf, 0x1fe);
	transfer_sizes[1] = usbDevWrite(tmpbuf, 0x300);
	transfer_sizes[2] = usbDevWrite(tmpbuf, 0x800);
	transfer_sizes[3] = usbDevWrite(tmpbuf, 0x1000);
	transfer_sizes[4] = usbDevWrite(tmpbuf, 0x2200);
	transfer_sizes[5] = usbDevWrite(tmpbuf, 0x10000);

	usbDevWrite(transfer_sizes, sizeof(transfer_sizes));

	svcSleepThread(5000000000);//Delay 5s

	return 0;
}

int main(int argc, char **argv)
{
	Result ret;

	ret = usbDevInitialize();

	if (R_SUCCEEDED(ret)) {
		u8 *tmpbuf = malloc(0x10000);
		if (tmpbuf==NULL) ret = -4;

		if (R_SUCCEEDED(ret)) {
			memset(tmpbuf, 0, 0x10000);
			ret = usbdev_test(tmpbuf);
		}

		usbDevExit();
	}

        if(R_FAILED(ret))fatalSimple(ret);

	svcSleepThread(5000000000);//Delay 5s

	return 0;
}

