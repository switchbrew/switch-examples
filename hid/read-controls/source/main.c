#include <string.h>
#include <stdio.h>

#include <switch.h>

//See also libnx hid.h.

int main(int argc, char **argv)
{
    //Matrix containing the name of each key. Useful for printing when a key is pressed
    char keysNames[32][32] = {
        "KEY_A", "KEY_B", "KEY_X", "KEY_Y",
        "KEY_LSTICK", "KEY_RSTICK", "KEY_L", "KEY_R",
        "KEY_ZL", "KEY_ZR", "KEY_PLUS", "KEY_MINUS",
        "KEY_DLEFT", "KEY_DUP", "KEY_DRIGHT", "KEY_DDOWN",
        "KEY_LSTICK_LEFT", "KEY_LSTICK_UP", "KEY_LSTICK_RIGHT", "KEY_LSTICK_DOWN",
        "KEY_RSTICK_LEFT", "KEY_RSTICK_UP", "KEY_RSTICK_RIGHT", "KEY_RSTICK_DOWN",
        "KEY_SL", "KEY_SR", "KEY_TOUCH", "",
        "", "", "", ""
    };

    consoleInit(NULL);

    u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

    printf("\x1b[1;1HPress PLUS to exit.");
    printf("\x1b[2;1HLeft joystick position:");
    printf("\x1b[4;1HRight joystick position:");

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        //hidKeysHeld returns information about which buttons have are held down in this frame
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        //hidKeysUp returns information about which buttons have been just released
        u64 kUp = hidKeysUp(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        //Do the keys printing only if keys have changed
        if (kDown != kDownOld || kHeld != kHeldOld || kUp != kUpOld)
        {
            //Clear console
            consoleClear();

            //These two lines must be rewritten because we cleared the whole console
            printf("\x1b[1;1HPress PLUS to exit.");
            printf("\x1b[2;1HLeft joystick position:");
            printf("\x1b[4;1HRight joystick position:");

            printf("\x1b[6;1H"); //Move the cursor to the sixth row because on the previous ones we'll write the joysticks' position

            //Check if some of the keys are down, held or up
            int i;
            for (i = 0; i < 32; i++)
            {
                if (kDown & BIT(i)) printf("%s down\n", keysNames[i]);
                if (kHeld & BIT(i)) printf("%s held\n", keysNames[i]);
                if (kUp & BIT(i)) printf("%s up\n", keysNames[i]);
            }
        }

        //Set keys old values for the next frame
        kDownOld = kDown;
        kHeldOld = kHeld;
        kUpOld = kUp;

        JoystickPosition pos_left, pos_right;

        //Read the joysticks' position
        hidJoystickRead(&pos_left, CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
        hidJoystickRead(&pos_right, CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);

        //Print the joysticks' position
        printf("\x1b[3;1H%04d; %04d", pos_left.dx, pos_left.dy);
        printf("\x1b[5;1H%04d; %04d", pos_right.dx, pos_right.dy);

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
