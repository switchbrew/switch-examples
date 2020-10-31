#include <switch.h>

void userAppInit(void)
{
    Result res = romfsInit();
    if (R_FAILED(res))
        diagAbortWithResult(res);
}

void userAppExit(void)
{
    romfsExit();
}
