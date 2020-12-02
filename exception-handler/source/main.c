// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

//This example shows how to use userland exception handling. See also libnx init.c and thread_context.h.

alignas(16) u8 __nx_exception_stack[0x1000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

void __libnx_exception_handler(ThreadExceptionDump *ctx)
{
    int i;
    FILE *f = fopen("exception_dump", "w");
    if(f==NULL)return;

    fprintf(f, "error_desc: 0x%x\n", ctx->error_desc);//You can also parse this with ThreadExceptionDesc.
    //This assumes AArch64, however you can also use threadExceptionIsAArch64().
    for(i=0; i<29; i++)fprintf(f, "[X%d]: 0x%lx\n", i, ctx->cpu_gprs[i].x);
    fprintf(f, "fp: 0x%lx\n", ctx->fp.x);
    fprintf(f, "lr: 0x%lx\n", ctx->lr.x);
    fprintf(f, "sp: 0x%lx\n", ctx->sp.x);
    fprintf(f, "pc: 0x%lx\n", ctx->pc.x);

    //You could print fpu_gprs if you want.

    fprintf(f, "pstate: 0x%x\n", ctx->pstate);
    fprintf(f, "afsr0: 0x%x\n", ctx->afsr0);
    fprintf(f, "afsr1: 0x%x\n", ctx->afsr1);
    fprintf(f, "esr: 0x%x\n", ctx->esr);

    fprintf(f, "far: 0x%lx\n", ctx->far.x);

    fclose(f);
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("Triggering crash...\n");
    consoleUpdate(NULL);

    //Trigger a crash.
    *((u64*)8) = 16;

    // Main loop
    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Your code goes here

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
