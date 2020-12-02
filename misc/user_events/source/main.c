#include <stdio.h>
#include <stdarg.h>
#include <switch.h>

static UEvent g_Event;
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
        rc = waitMulti(&idx, -1, waiterForUEvent(&g_Event), waiterForUEvent(&g_ExitEvent));
        locked_printf("waitMulti returned %x, index triggered = %d\n", rc, idx);

        if (R_SUCCEEDED(rc))
        {
            if (idx == 0)
            {
                locked_printf("g_Event triggered!\n");
                ueventClear(&g_Event);
            }
            else {
                locked_printf("g_ExitEvent triggered!\n");
                break;
            }
        }
    }

    rc = waitMulti(&idx, 0, waiterForUEvent(&g_Event), waiterForUEvent(&g_ExitEvent));

    if (R_SUCCEEDED(rc))
        locked_printf("Got leftover event %u\n", idx);
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
    ueventCreate(&g_Event, false);
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
            size_t i;
            for (i=0; i<5; i++) {
                locked_printf("Sleeping for a while\n");
                svcSleepThread(1000000000ull); // 1s

                locked_printf("Fire!\n");
                ueventSignal(&g_Event);
            }

            ueventSignal(&g_ExitEvent);
            threadWaitForExit(&thread);
        }

        threadClose(&thread);
    }

    while (appletMainLoop())
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
