#include <string.h>
#include <stdio.h>
#include <time.h>

#include <switch.h>

const char* const months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

const char* const weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("\x1b[16;16HPress PLUS to exit.");

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        //Print current time
        time_t unixTime = time(NULL);
        struct tm* timeStruct = gmtime((const time_t *)&unixTime);//Gets UTC time. If you want local-time use localtime().

        int hours = timeStruct->tm_hour;
        int minutes = timeStruct->tm_min;
        int seconds = timeStruct->tm_sec;
        int day = timeStruct->tm_mday;
        int month = timeStruct->tm_mon;
        int year = timeStruct->tm_year +1900;
        int wday = timeStruct->tm_wday;

        printf("\x1b[1;1H%02i:%02i:%02i", hours, minutes, seconds);
        printf("\n%s %s %i %i", weekDays[wday], months[month], day, year);

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
