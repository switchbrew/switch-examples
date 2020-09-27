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

    printf("\x1b[16;16HPress PLUS to exit.");

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

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
