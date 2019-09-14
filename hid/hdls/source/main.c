// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use Hdls for virtual HID controllers, see also libnx hiddbg.h.
// The virtual controllers can be used used by all processes.

// Main program entrypoint
int main(int argc, char* argv[])
{
    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    Result rc=0, rc2=0;
    bool initflag=0;
    u32 i;

    printf("hdls example\n");

    hidScanInput();

    // When hiddbgAttachHdlsVirtualDevice runs a new controller will become available. If CONTROLLER_HANDHELD is being internally, CONTROLLER_P1_AUTO would use the new virtual controller. Check which controller we're currently using and don't use CONTROLLER_P1_AUTO, so it doesn't switch to using the new controller later.
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

    u64 HdlsHandle=0;
    HiddbgHdlsDeviceInfo device = {0};
    HiddbgHdlsState state={0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    device.deviceType = HidDeviceType_FullKey3;
    device.npadInterfaceType = NpadInterfaceType_Bluetooth;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    device.singleColorBody = RGBA8_MAXALPHA(255,255,255);
    device.singleColorButtons = RGBA8_MAXALPHA(0,0,0);
    device.colorLeftGrip = RGBA8_MAXALPHA(230,255,0);
    device.colorRightGrip = RGBA8_MAXALPHA(0,40,20);

    // Setup example controller state.
    state.batteryCharge = 4; // Set battery charge to full.
    state.joysticks[JOYSTICK_LEFT].dx = 0x1234;
    state.joysticks[JOYSTICK_LEFT].dy = -0x1234;
    state.joysticks[JOYSTICK_RIGHT].dx = 0x5678;
    state.joysticks[JOYSTICK_RIGHT].dy = -0x5678;

    if (initflag) {
        rc = hiddbgAttachHdlsWorkBuffer();
        printf("hiddbgAttachHdlsWorkBuffer(): 0x%x\n", rc);

        if (R_SUCCEEDED(rc)) {
            // Attach a new virtual controller.
            rc = hiddbgAttachHdlsVirtualDevice(&HdlsHandle, &device);
            printf("hiddbgAttachHdlsVirtualDevice(): 0x%x\n", rc);
        }
    }

    printf("Press A to scan controllers.\n");
    printf("Press + to exit.\n");

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(conID);

        // Set state for the controller. You can also use hiddbgApplyHdlsStateList for this.
        if (R_SUCCEEDED(rc)) {
            rc2 = hiddbgSetHdlsState(HdlsHandle, &state);
            if (R_FAILED(rc2)) printf("hiddbgSetHdlsState(): 0x%x\n", rc2);

            state.buttons = 0;

            if (hidKeysHeld(conID) & KEY_R)
                state.buttons |= KEY_HOME;

            if (hidKeysHeld(conID) & KEY_L)
                state.buttons |= KEY_CAPTURE;

            if (hidKeysHeld(conID) & KEY_DUP)
                state.buttons |= KEY_ZR;

            state.joysticks[JOYSTICK_LEFT].dx += 0x10;
            if (state.joysticks[JOYSTICK_LEFT].dx > JOYSTICK_MAX) state.joysticks[JOYSTICK_LEFT].dx = JOYSTICK_MIN;
            state.joysticks[JOYSTICK_RIGHT].dy -= 0x10;
            if (state.joysticks[JOYSTICK_LEFT].dy < JOYSTICK_MIN) state.joysticks[JOYSTICK_LEFT].dy = JOYSTICK_MAX;
        }

        if (R_SUCCEEDED(rc) && (kDown & (KEY_A | KEY_X))) {
            printf("Connected controllers:\n");
            for(i=0; i<10; i++) {
                if (hidIsControllerConnected(i)) {
                    JoystickPosition tmpjoy[2];
                    hidJoystickRead(&tmpjoy[0], i, JOYSTICK_LEFT);
                    hidJoystickRead(&tmpjoy[1], i, JOYSTICK_RIGHT);

                    u8 interfacetype=0;
                    rc2 = hidGetNpadInterfaceType(i, &interfacetype);
                    if (R_FAILED(rc2)) printf("hidGetNpadInterfaceType(): 0x%x\n", rc2);

                    HidPowerInfo powerinfo[3]={0};
                    hidGetControllerPowerInfo(i, &powerinfo[0], 1);
                    hidGetControllerPowerInfo(i, &powerinfo[1], 2);

                    printf("%d: type = 0x%x, devicetype = 0x%x, buttons = 0x%lx, stickL.dx = 0x%x, stickL.dy = 0x%x, stickR.dx = 0x%x, stickR.dy = 0x%x, interface = %d\n", i, hidGetControllerType(i), hidGetControllerDeviceType(i), hidKeysHeld(i), tmpjoy[0].dx, tmpjoy[0].dy, tmpjoy[1].dx, tmpjoy[1].dy, interfacetype);

                    for (u32 poweri=0; poweri<3; poweri++)
                        printf("%d powerinfo[%d]: powerConnected = %d, isCharging = %d, batteryCharge = %d\n", i, poweri, powerinfo[poweri].powerConnected, powerinfo[poweri].isCharging, powerinfo[poweri].batteryCharge);
                }
            }
            printf("\n");
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu
    }

    if (initflag) {
        // These *must* be run at some point before exiting.

        if (R_SUCCEEDED(rc)) {
            rc = hiddbgDetachHdlsVirtualDevice(HdlsHandle);
            printf("hiddbgDetachHdlsVirtualDevice(): 0x%x\n", rc);
        }

        rc = hiddbgReleaseHdlsWorkBuffer();
        printf("hiddbgReleaseHdlsWorkBuffer(): 0x%x\n", rc);

        hiddbgExit();
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
