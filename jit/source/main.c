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

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    rc = jitCreate(&j, 0x100000);//Adjust size as needed.
    printf("jitCreate() returned: 0x%x\n", rc);
    printf("jit type: %d\n", j.type);

    if (R_SUCCEEDED(rc))
    {
        jit_rwaddr = jitGetRwAddr(&j);
        jit_rxaddr = jitGetRxAddr(&j);
        printf("jitGetRwAddr(): %p\n", jit_rwaddr);
        printf("jitGetRxAddr(): %p\n", jit_rxaddr);

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
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // Your code goes here

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
