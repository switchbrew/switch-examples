#include <string.h>
#include <stdio.h>
#include <time.h>

#include <switch.h>

const char* const chargers[3] = {"None", "Official", "Generic"};

int main(int argc, char **argv)
{
    consoleInit(NULL);

    Result rc = psmInitialize();
    if (R_FAILED(rc))
        goto cleanup;

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("\x1b[16;16HPress PLUS to exit.");

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        u32 charge;
        double rawCharge;
        double age;
        bool isEnoughPower;
        ChargerType type;
        printf("\x1b[1;1H\x1b[K");

        rc = psmGetBatteryChargePercentage(&charge);
        if (R_FAILED(rc))
        {
            printf("psmGetBatteryChargePercentage: %08X", rc);
            continue;
        }

        rc = psmGetRawBatteryChargePercentage(&rawCharge);
        if (R_FAILED(rc))
        {
            printf("psmGetRawBatteryChargePercentage: %08X", rc);
            continue;
        }

        rc = psmGetBatteryAgePercentage(&age);
        if (R_FAILED(rc))
        {
            printf("psmGetBatteryAgePercentage: %08X", rc);
            continue;
        }

        rc = psmIsEnoughPowerSupplied(&isEnoughPower);
        if (R_FAILED(rc))
        {
            printf("psmIsEnoughPower: %08X", rc);
            continue;
        }

        rc = psmGetChargerType(&type);
        if (R_FAILED(rc))
        {
            printf("psmGetChargerType: %08X", rc);
            continue;
        }

        printf("Battery charge: %u%% (%1.2f%%)", charge, rawCharge);
        printf("\n\x1b[KBattery age: %1.2f%%", age);
        printf("\n\x1b[KHas enough power: %s", isEnoughPower ? "Yes" : "No");
        printf("\n\x1b[KCharger type: %s", chargers[type]);

        consoleUpdate(NULL);
    }

cleanup:
    psmExit();
    consoleExit(NULL);
    return 0;
}
