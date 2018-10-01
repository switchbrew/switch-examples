#include <string.h>
#include <stdio.h>

#include <switch.h>

//Example for using JIT. See also libnx jit.h.

int main(int argc, char **argv)
{
    Result rc=0;
    Jit j;
    u8* jit_rwaddr = NULL, *jit_rxaddr = NULL;
    u64 (*funcptr)(void);
    u32 testcode[2] = {0xd2800000 | (0x7<<5), 0xd65f03c0};//"mov x0, #0x7" "ret"

    consoleInit(NULL);

    rc = jitCreate(&j, 0x100000);//Adjust size as needed.
    printf("jitCreate() returned: 0x%x\n", rc);

    if (R_SUCCEEDED(rc))
    {
        jit_rwaddr = jitGetRwAddr(&j);
        jit_rxaddr = jitGetRxAddr(&j);

        rc = jitTransitionToWritable(&j);
        printf("jitTransitionToWritable() returned: 0x%x\n", rc);

        if (R_SUCCEEDED(rc))
        {
            memcpy(jit_rwaddr, testcode, sizeof(testcode));

            rc = jitTransitionToExecutable(&j);
            printf("jitTransitionToExecutable() returned: 0x%x\n", rc);

            if (R_SUCCEEDED(rc))
            {
                funcptr = (void*)jit_rxaddr;
                printf("Test code returned: 0x%lx\n", funcptr());//This should return 0x7 (see above).
            }
        }

        rc = jitClose(&j);
        printf("jitClose() returned: 0x%x\n", rc);
    }

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // Your code goes here

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
