#include <stdio.h>
#include <switch.h>

static UsermodeEvent g_Event;
static UsermodeEvent g_ExitEvent;
static Mutex g_PrintMutex;

void locked_print(const char* str) {
    mutexLock(&g_PrintMutex);
    printf(str);
    mutexUnlock(&g_PrintMutex);
}

void threadFunc1(void* arg)
{
    Result rc;
    int idx;

    locked_print("Entering thread\n");

    while (1)
    {
        rc = waitMulti(&idx, -1, waiterForUevent(&g_Event), waiterForUevent(&g_ExitEvent));

        mutexLock(&g_PrintMutex);
        printf("waitMulti returned %x, index triggered = %d\n", rc, idx);
        mutexUnlock(&g_PrintMutex);

        if (R_SUCCEEDED(rc))
        {
            if (idx == 0)
            {
                locked_print("g_Event triggered!\n");
                ueventClear(&g_Event);
            }
            else {
                locked_print("g_ExitEvent triggered!\n");
                break;
            }
        }
    }

    rc = waitMulti(&idx, 0, waiterForUevent(&g_Event), waiterForUevent(&g_ExitEvent));

    if (R_SUCCEEDED(rc))
    {
        mutexLock(&g_PrintMutex);
        printf("Got leftover event %u\n", idx);
        mutexUnlock(&g_PrintMutex);
    }
}

int main(int argc, char **argv)
{
    gfxInitDefault();
    consoleInit(NULL);

    mutexInit(&g_PrintMutex);
    ueventCreate(&g_Event, false);
    ueventCreate(&g_ExitEvent, false);

    locked_print("Creating thread\n");

    Thread thread;
    Result rc;
    rc = threadCreate(&thread, (ThreadFunc) threadFunc1, NULL, 0x1000, 0x2C, -2);

    if (R_SUCCEEDED(rc))
    {
        rc = threadStart(&thread);

        if (R_SUCCEEDED(rc))
        {
            size_t i;
            for (i=0; i<5; i++) {
                locked_print("Sleeping for a while\n");
                svcSleepThread(1000000000ull); // 1s

                locked_print("Fire!\n");
                ueventSignal(&g_Event);
            }

            ueventSignal(&g_ExitEvent);
            threadWaitForExit(&thread);
        }

        threadClose(&thread);
    }

    while (appletMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}
