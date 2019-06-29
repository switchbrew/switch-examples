// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use AbstractedPad, see also libnx hiddbg.h. Depending on state type2, either a new virtual controller can be registered, or the state can be merged with an existing controller.
// Note that Hdls can also be used for this, which generally you would want to use that instead.

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    Result rc=0;
    bool initflag=0;
    u32 i;

    printf("abstracted-pad example\n");

    hidScanInput();

    // When hiddbgSetAutoPilotVirtualPadState runs a new controller may become available, depending on the specified type2 field. If CONTROLLER_HANDHELD is being internally, CONTROLLER_P1_AUTO would use the new controller. Check which controller we're currently using and don't use CONTROLLER_P1_AUTO, so it doesn't switch to using the new controller later.
    HidControllerID conID = hidGetHandheldMode() ? CONTROLLER_HANDHELD : CONTROLLER_PLAYER_1;

    printf("Connected controllers: ");
    for(i=0; i<10; i++) {
        if (hidIsControllerConnected(i)) printf("%d ", i);
    }
    printf("\n");

    rc = hiddbgInitialize();
    if (R_FAILED(rc)) {
        printf("hiddbgInitialize(): 0x%x\n", rc);
    }
    else {
        initflag = 1;
    }

    u64 pads[8]={0};
    HiddbgAbstractedPadState states[8]={0};
    s32 tmpout=0;

    // You can also use hiddbgGetAbstractedPadHandles + hiddbgGetAbstractedPadState if you want.

    printf("Press A to scan controllers and update state.\n");
    printf("Press X to scan controllers.\n");
    printf("Press + to exit.\n");

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(conID);

        if (R_SUCCEEDED(rc) && (kDown & (KEY_A | KEY_X))) {
            printf("Connected controllers:\n");
            for(i=0; i<10; i++) {
                if (hidIsControllerConnected(i)) {
                    JoystickPosition tmpjoy[2];
                    hidJoystickRead(&tmpjoy[0], i, JOYSTICK_LEFT);
                    hidJoystickRead(&tmpjoy[1], i, JOYSTICK_RIGHT);
                    printf("%d: type = 0x%x, devicetype = 0x%x, buttons = 0x%lx, stickL.dx = 0x%x, stickL.dy = 0x%x, stickR.dx = 0x%x, stickR.dy = 0x%x\n", i, hidGetControllerType(i), hidGetControllerDeviceType(i), hidKeysHeld(i), tmpjoy[0].dx, tmpjoy[0].dy, tmpjoy[1].dx, tmpjoy[1].dy);
                }
            }
        }

        if (R_SUCCEEDED(rc) && (kDown & KEY_A)) {
            tmpout = 0;
            rc = hiddbgGetAbstractedPadsState(pads, states, sizeof(pads)/sizeof(u64), &tmpout);
            printf("hiddbgGetAbstractedPadsState(): 0x%x, 0x%x\n", rc, tmpout);

            // Note that each pad/state corresponds with a single Joy-Con / controller.

            if (R_SUCCEEDED(rc) && tmpout>=1) {
                for (u32 i=0; i<tmpout; i++) {
                    printf("0x%x: \n", i);
                    printf("type = 0x%x, flags = 0x%x, colors = 0x%x 0x%x, type2 = 0x%x, buttons = 0x%x, stickL.dx = 0x%x, stickL.dy = 0x%x, stickR.dx = 0x%x, stickR.dy = 0x%x\n", states[i].type, states[i].flags, states[i].singleColorBody, states[i].singleColorButtons, states[i].type2, states[i].state.buttons, states[i].state.joysticks[0].dx, states[i].state.joysticks[0].dy, states[i].state.joysticks[1].dx, states[i].state.joysticks[1].dy);
                }
            }

            if (tmpout>=1) {
                s8 AbstractedVirtualPadId=0;

                // Setup state. You could also construct it without using hiddbgGetAbstractedPadsState, if preferred.

                // Set type to one that's usable with state-merge. Note that this is also available with Hdls.
                states[0].type = BIT(1);
                // Use state-merge for the above controller, the state will be merged with an existing controller.
                // For a plain virtual controller, use type2 value 0x0, and update the above type value.
                states[0].type2 = 0x2;

                states[0].state.buttons |= KEY_ZL;

                rc = hiddbgSetAutoPilotVirtualPadState(AbstractedVirtualPadId, &states[0]);
                printf("hiddbgSetAutoPilotVirtualPadState(): 0x%x\n", rc);
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu
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
