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

    // Configure our supported input layout: all players with standard controller styles
    padConfigureInput(8, HidNpadStyleSet_NpadStandard);

    // Initialize the gamepad for reading all controllers
    PadState pad;
    padInitializeAny(&pad);

    Result rc=0, rc2=0;
    bool initflag=0;

    printf("hdls example\n");

    rc = hiddbgInitialize();
    if (R_FAILED(rc)) {
        printf("hiddbgInitialize(): 0x%x\n", rc);
    }
    else {
        initflag = 1;
    }

    HiddbgHdlsHandle HdlsHandle={0};
    HiddbgHdlsDeviceInfo device = {0};
    HiddbgHdlsState state={0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    device.deviceType = HidDeviceType_FullKey3;
    device.npadInterfaceType = HidNpadInterfaceType_Bluetooth;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    device.singleColorBody = RGBA8_MAXALPHA(255,255,255);
    device.singleColorButtons = RGBA8_MAXALPHA(0,0,0);
    device.colorLeftGrip = RGBA8_MAXALPHA(230,255,0);
    device.colorRightGrip = RGBA8_MAXALPHA(0,40,20);

    // Setup example controller state.
    state.battery_level = 4; // Set BatteryLevel to full.
    state.analog_stick_l.x = 0x1234;
    state.analog_stick_l.y = -0x1234;
    state.analog_stick_r.x = 0x5678;
    state.analog_stick_r.y = -0x5678;

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
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Set state for the controller. You can also use hiddbgApplyHdlsStateList for this.
        if (R_SUCCEEDED(rc)) {
            rc2 = hiddbgSetHdlsState(HdlsHandle, &state);
            if (R_FAILED(rc2)) printf("hiddbgSetHdlsState(): 0x%x\n", rc2);

            state.buttons = 0;

            if (padGetButtons(&pad) & HidNpadButton_R)
                state.buttons |= HiddbgNpadButton_Home;

            if (padGetButtons(&pad) & HidNpadButton_L)
                state.buttons |= HiddbgNpadButton_Capture;

            if (padGetButtons(&pad) & HidNpadButton_Up)
                state.buttons |= HidNpadButton_ZR;

            state.analog_stick_l.x += 0x10;
            if (state.analog_stick_l.x > JOYSTICK_MAX) state.analog_stick_l.x = JOYSTICK_MIN;
            state.analog_stick_r.y -= 0x10;
            if (state.analog_stick_r.y < JOYSTICK_MIN) state.analog_stick_r.y = JOYSTICK_MAX;
        }

        if (R_SUCCEEDED(rc) && (kDown & (HidNpadButton_A | HidNpadButton_X))) {
            printf("Controllers state:\n");
            HidAnalogStickState analog_stick_l = padGetStickPos(&pad, 0);
            HidAnalogStickState analog_stick_r = padGetStickPos(&pad, 1);
            printf("buttons = 0x%lx, analog_stick_l.x = 0x%x, analog_stick_l.y = 0x%x, analog_stick_r.x = 0x%x, analog_stick_r.y = 0x%x\n", padGetButtons(&pad), analog_stick_l.x, analog_stick_l.y, analog_stick_r.x, analog_stick_r.y);
            printf("\n");
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
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
