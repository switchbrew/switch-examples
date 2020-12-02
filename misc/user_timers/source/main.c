#include <stdio.h>
#include <stdarg.h>
#include <switch.h>

static UTimer g_Timer;
static UTimer g_FastTimer;
static UEvent g_ExitEvent;
static Mutex g_PrintMutex;

__attribute__((format(printf, 1, 2)))
static void locked_printf(const char* fmt, ...)
{
    mutexLock(&g_PrintMutex);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    consoleUpdate(NULL);
    mutexUnlock(&g_PrintMutex);
}

void threadFunc1(void* arg)
{
    Result rc;
    int idx;

    locked_printf("Entering thread\n");

    while (1)
    {
        rc = waitMulti(&idx, -1, waiterForUTimer(&g_Timer), waiterForUTimer(&g_FastTimer), waiterForUEvent(&g_ExitEvent));

        if (R_SUCCEEDED(rc))
        {
            if (idx == 0) {
                locked_printf("g_Timer triggered!\n");
            }
            else if (idx == 1) {
                locked_printf("g_FasterTimer triggered!\n");
            }
            else {
                locked_printf("g_ExitEvent triggered!\n");
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    mutexInit(&g_PrintMutex);
    utimerCreate(&g_Timer, 2000000000, TimerType_Repeating); // 2s
    utimerCreate(&g_FastTimer, 1000000000, TimerType_Repeating); // 1s
    ueventCreate(&g_ExitEvent, false);

    locked_printf("Creating thread\n");

    Thread thread;
    Result rc;
    rc = threadCreate(&thread, threadFunc1, NULL, NULL, 0x10000, 0x2C, -2);

    if (R_SUCCEEDED(rc))
    {
        rc = threadStart(&thread);

        if (R_SUCCEEDED(rc))
        {
            locked_printf("Starting timer for 5s\n");
            utimerStart(&g_Timer);
            utimerStart(&g_FastTimer);
            svcSleepThread(5000000000ull); // 5s

            locked_printf("Stopping timer for 5s\n");
            utimerStop(&g_Timer);
            utimerStop(&g_FastTimer);
            svcSleepThread(5000000000ull); // 5s

            locked_printf("Starting timer for 5s\n");
            utimerStart(&g_Timer);
            utimerStart(&g_FastTimer);
            svcSleepThread(5000000000ull); // 5s

            locked_printf("Done\n");
            ueventSignal(&g_ExitEvent);

            threadWaitForExit(&thread);
        }

        threadClose(&thread);
    }

    while(appletMainLoop())
    {
        padUpdate(&pad);

        u32 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break;

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
