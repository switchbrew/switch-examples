#include <string.h>
#include <stdio.h>
#include <time.h>

#include <switch.h>

const char* const months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

const char* const weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const u16 daysAtStartOfMonthLUT[12] =
{
	0   % 7, //january    31
	31  % 7, //february   28+1(leap year)
	59  % 7, //march      31
	90  % 7, //april      30
	120 % 7, //may        31
	151 % 7, //june       30
	181 % 7, //july       31
	212 % 7, //august     31
	243 % 7, //september  30
	273 % 7, //october    31
	304 % 7, //november   30
	334 % 7  //december   31
};

static inline bool isLeapYear(int year)
{
	return (year%4) == 0 && !((year%100) == 0 && (year%400) != 0);
}

static inline int getDayOfWeek(int day, int month, int year)
{
	//http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week
	day += 2*(3-((year/100)%4));
	year %= 100;
	day += year + (year/4);
	day += daysAtStartOfMonthLUT[month] - (isLeapYear(year) && (month <= 1));
	return day % 7;
}

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(NULL);

	printf("\x1b[16;16HPress Start to exit.");

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
		struct tm* timeStruct = gmtime((const time_t *)&unixTime);

		int hours = timeStruct->tm_hour;
		int minutes = timeStruct->tm_min;
		int seconds = timeStruct->tm_sec;
		int day = timeStruct->tm_mday;
		int month = timeStruct->tm_mon;
		int year = timeStruct->tm_year +1900;

		printf("\x1b[1;1H%02i:%02i:%02i", hours, minutes, seconds);
		printf("\n%s %s %i %i", weekDays[getDayOfWeek(day, month, year)], months[month], day, year);

		gfxFlushBuffers();
		gfxSwapBuffers();
		gfxWaitForVsync();
	}

	gfxExit();
	return 0;
}

