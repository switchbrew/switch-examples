/*
** Sample Framework for deko3d Applications
**   startup.cpp: Automatic initialization/deinitialization
*/
#include "common.h"
#include <unistd.h>

//#define DEBUG_NXLINK

#ifdef DEBUG_NXLINK
static int nxlink_sock = -1;
#endif

extern "C" void userAppInit(void)
{
    Result res = romfsInit();
    if (R_FAILED(res))
        fatalThrow(res);

#ifdef DEBUG_NXLINK
    socketInitializeDefault();
    nxlink_sock = nxlinkStdioForDebug();
#endif
}

extern "C" void userAppExit(void)
{
#ifdef DEBUG_NXLINK
    if (nxlink_sock != -1)
        close(nxlink_sock);
    socketExit();
#endif

    romfsExit();
}
