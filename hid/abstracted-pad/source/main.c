// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use AbstractedPad, see also libnx hiddbg.h. Depending on state npadInterfaceType, either a new virtual controller can be registered, or the state can be merged with an existing controller.
// This is deprecated, use Hdls instead when running on compatible system-versions.

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    // Configure our supported input layout: all players with standard controller styles
    padConfigureInput(8, HidNpadStyleSet_NpadStandard);

    // Initialize the gamepad for reading all controllers
    PadState pad;
    padInitializeAny(&pad);

    Result rc=0;
    bool initflag=0;

    printf("abstracted-pad example\n");

    rc = hiddbgInitialize();
    if (R_FAILED(rc)) {
        printf("hiddbgInitialize(): 0x%x\n", rc);
    }
    else {
        initflag = 1;
    }

    HiddbgAbstractedPadHandle pads[8]={0};
    HiddbgAbstractedPadState states[8]={0};
    s32 tmpout=0;

    // You can also use hiddbgGetAbstractedPadHandles + hiddbgGetAbstractedPadState if you want.

    printf("Press A to scan controllers and update state.\n");
    printf("Press X to scan controllers.\n");
    printf("Press + to exit.\n");

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

        if (R_SUCCEEDED(rc) && (kDown & (HidNpadButton_A | HidNpadButton_X))) {
            printf("Controllers state:\n");
            HidAnalogStickState analog_stick_l = padGetStickPos(&pad, 0);
            HidAnalogStickState analog_stick_r = padGetStickPos(&pad, 1);
            printf("buttons = 0x%lx, analog_stick_l.x = 0x%x, analog_stick_l.y = 0x%x, analog_stick_r.x = 0x%x, analog_stick_r.y = 0x%x\n", padGetButtons(&pad), analog_stick_l.x, analog_stick_l.y, analog_stick_r.x, analog_stick_r.y);
        }

        if (R_SUCCEEDED(rc) && (kDown & HidNpadButton_A)) {
            tmpout = 0;
            rc = hiddbgGetAbstractedPadsState(pads, states, sizeof(pads)/sizeof(u64), &tmpout);
            printf("hiddbgGetAbstractedPadsState(): 0x%x, 0x%x\n", rc, tmpout);

            // Note that each pad/state corresponds with a single Joy-Con / controller.

            if (R_SUCCEEDED(rc) && tmpout>=1) {
                for (u32 i=0; i<tmpout; i++) {
                    printf("0x%x: \n", i);
                    printf("type = 0x%x, flags = 0x%x, colors = 0x%x 0x%x, npadInterfaceType = 0x%x, buttons = 0x%x, analog_stick_l.x = 0x%x, analog_stick_l.y = 0x%x, analog_stick_r.x = 0x%x, analog_stick_r.y = 0x%x\n", states[i].type, states[i].flags, states[i].singleColorBody, states[i].singleColorButtons, states[i].npadInterfaceType, states[i].state.buttons, states[i].state.analog_stick_l.x, states[i].state.analog_stick_l.y, states[i].state.analog_stick_r.x, states[i].state.analog_stick_r.y);
                }
            }

            if (tmpout>=1) {
                s8 AbstractedVirtualPadId=0;

                // Setup state. You could also construct it without using hiddbgGetAbstractedPadsState, if preferred.

                // Set type to one that's usable with state-merge. Note that this is also available with Hdls.
                states[0].type = BIT(1);
                // Use state-merge for the above controller, the state will be merged with an existing controller.
                // For a plain virtual controller, use NpadInterfaceType_Bluetooth, and update the above type value.
                states[0].npadInterfaceType = HidNpadInterfaceType_Rail;

                states[0].state.buttons |= HidNpadButton_ZL;

                rc = hiddbgSetAutoPilotVirtualPadState(AbstractedVirtualPadId, &states[0]);
                printf("hiddbgSetAutoPilotVirtualPadState(): 0x%x\n", rc);
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    if (initflag) {
        // These *must* be run at some point before exiting.

        // You can also use hiddbgUnsetAutoPilotVirtualPadState with a specific AbstractedVirtualPadId if you want.

        rc = hiddbgUnsetAllAutoPilotVirtualPadState();
        printf("hiddbgUnsetAllAutoPilotVirtualPadState(): 0x%x\n", rc);

        hiddbgExit();
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
