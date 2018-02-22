#include <string.h>
#include <stdio.h>
#include <time.h>

#include <switch.h>

const char* const months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

const char* const weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(NULL);

	printf("\x1b[16;16HPress PLUS to exit.");

	// Main loop
	while(appletMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

		//Print current time
		time_t unixTime = time(NULL);
		struct tm* timeStruct = gmtime((const time_t *)&unixTime);//Gets UTC time. Currently localtime() will also return UTC (timezones not supported).

		int hours = timeStruct->tm_hour;
		int minutes = timeStruct->tm_min;
		int seconds = timeStruct->tm_sec;
		int day = timeStruct->tm_mday;
		int month = timeStruct->tm_mon;
		int year = timeStruct->tm_year +1900;
		int wday = timeStruct->tm_wday;

		printf("\x1b[1;1H%02i:%02i:%02i", hours, minutes, seconds);
		printf("\n%s %s %i %i", weekDays[wday], months[month], day, year);

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

