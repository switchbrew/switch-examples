#include <switch.h>

void userAppInit(void)
{
    Result res = romfsInit();
    if (R_FAILED(res))
        fatalThrow(res);
}

void userAppExit(void)
{
    romfsExit();
}
