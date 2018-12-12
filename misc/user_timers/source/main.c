#include <stdio.h>
#include <switch.h>

static UsermodeTimer g_Timer;
static UsermodeTimer g_FastTimer;
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
        rc = waitMulti(&idx, -1, waiterForUtimer(&g_Timer), waiterForUtimer(&g_FastTimer), waiterForUevent(&g_ExitEvent));

        if (R_SUCCEEDED(rc))
        {
            if (idx == 0) {
                locked_print("g_Timer triggered!\n");
            }
            else if (idx == 1) {
                locked_print("g_FasterTimer triggered!\n");
            }
            else {
                locked_print("g_ExitEvent triggered!\n");
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    gfxInitDefault();
    consoleInit(NULL);

    mutexInit(&g_PrintMutex);
    utimerCreate(&g_Timer, 2000000000, true); // 2s
    utimerCreate(&g_FastTimer, 1000000000, true); // 1s
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
            locked_print("Sleeping for 5s\n");
            svcSleepThread(5000000000ull); // 5s

            locked_print("Stopping timer for 5s\n");
            utimerStop(&g_Timer);
            utimerStop(&g_FastTimer);
            svcSleepThread(5000000000ull); // 5s

            locked_print("Starting timer for 5s\n");
            utimerStart(&g_Timer);
            utimerStart(&g_FastTimer);
            svcSleepThread(5000000000ull); // 5s

            locked_print("Done\n");
            ueventSignal(&g_ExitEvent);

            threadWaitForExit(&thread);
        }

        threadClose(&thread);
    }

    while(appletMainLoop())
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
