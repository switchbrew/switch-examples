#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <switch.h>

static Mutex g_PrintMutex;
static Barrier g_Barrier;

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

void threadFunc(void* arg)
{
    u64 num = (u64) arg;

    u64 i;
    for (i=0; i<2; i++)
    {
        locked_printf("Entering barrier %" PRIu64 "\n", num);
        barrierWait(&g_Barrier);
        locked_printf("Leaving barrier %" PRIu64 "\n", num);
    }
}

int main(int argc, char **argv)
{
    consoleInit(NULL);

    mutexInit(&g_PrintMutex);
    barrierInit(&g_Barrier, 4);

    locked_printf("Creating threads\n");

    static Thread thread[4];
    int num_threads;
    Result rc;

    u64 i;
    for (i=0; i<4; i++) {
        num_threads = i;
        rc = threadCreate(&thread[i], threadFunc, (void*)i, 0x10000, 0x2C, -2);
        if (R_FAILED(rc))
            goto clean_up;
    }
    num_threads = i+1;

    for (i=0; i<4; i++) {
        rc = threadStart(&thread[i]);
        if (R_FAILED(rc))
            goto clean_up;
    }

    while(appletMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break;

        mutexLock(&g_PrintMutex);
        consoleUpdate(NULL);
        mutexUnlock(&g_PrintMutex);
    }

clean_up:
    for (i=0; i<num_threads; i++) {
        threadWaitForExit(&thread[i]);
        threadClose(&thread[i]);
    }
    consoleExit(NULL);
    return 0;
}
