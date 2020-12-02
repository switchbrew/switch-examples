// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use Alarm Notifications, see libnx notif.h.
// This must be run under a host Application. To avoid conflicts with official apps, normally you should not run under a host Application where the official Application also uses Alarm Notifications.

// Parse the ApplicationParameter and verify that it belongs to the current hb app. You can store any data you want in the ApplicationParameter.
bool parse_app_param(u8 *outdata, size_t outdata_size, u64 out_size, u8 *app_param_header, size_t app_param_header_size) {
    printf("out_size=0x%lx\n", out_size);
    if (out_size != outdata_size) // Adjust this check if needed.
        printf("out_size is invalid, ignoring data since it doesn't belong to the current app.\n");
    else {
        if(memcmp(outdata, app_param_header, app_param_header_size)==0) {
            printf("data: %s\n", (char*)&outdata[0x8]);
            return true;
        }
        else
            printf("ApplicationParameter header mismatch, ignoring data since it doesn't belong to the current app.\n");
    }

    return false;
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

    printf("alarm-notifications example\n");

    Result rc=0, rc2=0;
    bool tmpflag=0;
    u16 alarm_setting_id=0;
    s32 total_out=0;
    NotifAlarmSetting alarm_setting;
    NotifAlarmSetting alarm_settings[NOTIF_MAX_ALARMS];
    Event alarmevent={0};

    // To avoid conflicts with other hb apps, use some sort of header to verify that the ApplicationParameter belongs to the current hb app. Replace this with your own data. Note that official apps don't do this.
    // Note that ApplicationParameter is optional, see notif.h - you can disable/remove code using it you want.
    u8 app_param_header[0x8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    u8 app_param[0x20]={0};

    u8 outdata[0x21]; // Since we're handling this as a string, make sure it's NUL-terminated.

    memcpy(app_param, app_param_header, sizeof(app_param_header));
    strncpy((char*)&app_param[0x8], "app param", sizeof(app_param)-9); // This example stores a string in the app_param - replace this with whatever data you want.

    rc = notifInitialize(NotifServiceType_Application);
    if (R_FAILED(rc)) printf("notifInitialize(): 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) { // Some official apps don't use this. See libnx notif.h for this.
        rc = notifGetNotificationSystemEvent(&alarmevent);
        printf("notifGetNotificationSystemEvent(): 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        printf("Press A to register the Alarm.\n");
        printf("Press B to list the Alarms.\n");
        printf("Press X to delete the registered Alarm.\n");
    }
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

        if (R_SUCCEEDED(rc) && R_SUCCEEDED(eventWait(&alarmevent, 0))) { // Some official apps don't use this. See libnx notif.h for this.
            u64 out_size=0;
            memset(outdata, 0, sizeof(outdata));
            rc = notifTryPopNotifiedApplicationParameter(outdata, sizeof(outdata)-1, &out_size);
            printf("notifTryPopNotifiedApplicationParameter(): 0x%x\n", rc);
            if (R_SUCCEEDED(rc)) {
                parse_app_param(outdata, sizeof(outdata)-1, out_size, app_param_header, sizeof(app_param_header));
            }
        }

        if (R_SUCCEEDED(rc)) {
            if (kDown & HidNpadButton_A) {
                // Setup an alarm with {current local-time} + {2 minutes}. You can use any weekday/time you want.
                // See libnx notif.h for more notifAlarmSetting*() funcs.

                notifAlarmSettingCreate(&alarm_setting);

                time_t unixTime = time(NULL) + 60*2;
                struct tm* timeStruct = localtime((const time_t *)&unixTime);

                u32 day = timeStruct->tm_wday;
                s32 hour = timeStruct->tm_hour;
                s32 minute = timeStruct->tm_min;

                printf("Using the following schedule setting: weekday = %u, %02d:%02d.\n", day, hour, minute);

                // You can also use this multiple times if you want for multiple days - this example only uses it once for a single day.
                rc = notifAlarmSettingEnable(&alarm_setting, day, hour, minute);
                printf("notifAlarmSettingEnable(): 0x%x\n", rc);

                if (R_SUCCEEDED(rc)) {
                    rc = notifRegisterAlarmSetting(&alarm_setting_id, &alarm_setting, app_param, sizeof(app_param));
                    printf("notifRegisterAlarmSetting(): 0x%x\n", rc);
                    if (R_SUCCEEDED(rc)) printf("alarm_setting_id = 0x%x\n", alarm_setting_id);
                }
            }
            else if (kDown & HidNpadButton_B) {
                // List the Alarms.

                total_out=0;
                memset(alarm_settings, 0, sizeof(alarm_settings));
                rc = notifListAlarmSettings(alarm_settings, NOTIF_MAX_ALARMS, &total_out);
                printf("notifListAlarmSettings(): 0x%x\n", rc);
                if (R_SUCCEEDED(rc)) {
                    printf("total_out: %d\n", total_out);

                    for (s32 alarmi=0; alarmi<total_out; alarmi++) {
                        printf("[%d]: muted=%d.\n", alarmi, alarm_settings[alarmi].muted);

                        // Verify that the Alarm belongs to the current app via the ApplicationParameter / parse the data from there.
                        u32 actual_size=0;
                        memset(outdata, 0, sizeof(outdata));
                        rc2 = notifLoadApplicationParameter(alarm_settings[alarmi].alarm_setting_id, outdata, sizeof(outdata)-1, &actual_size);
                        printf("notifLoadApplicationParameter(): 0x%x\n", rc2);
                        if (R_FAILED(rc2) || !parse_app_param(outdata, sizeof(outdata)-1, actual_size, app_param_header, sizeof(app_param_header))) continue;

                        // If you want to update an AlarmSetting/ApplicationParameter from the alarm-listing, you can use this. notifAlarmSetting*() funcs can be used to update it if wanted.
                        // rc2 = notifUpdateAlarmSetting(&alarm_settings[alarmi], outdata, sizeof(outdata)-1);
                        // printf("notifUpdateAlarmSetting(): 0x%x\n", rc2);

                        // Print the schedule settings.
                        for (u32 day_of_week=0; day_of_week<7; day_of_week++) {
                            rc2 = notifAlarmSettingIsEnabled(&alarm_settings[alarmi], day_of_week, &tmpflag);
                            if (R_SUCCEEDED(rc2) && tmpflag) {
                                NotifAlarmTime alarmtime={0};
                                rc2 = notifAlarmSettingGet(&alarm_settings[alarmi], day_of_week, &alarmtime);
                                if (R_SUCCEEDED(rc2)) {
                                    printf("weekday %d = %02d:%02d. ", day_of_week, alarmtime.hour, alarmtime.minute);
                                }
                            }
                        }
                        printf("\n");
                    }
                }
            }
            else if ((kDown & HidNpadButton_X) && alarm_setting_id) {
                // Delete the AlarmSetting which was registered with the HidNpadButton_A block.
                // If wanted, you can also use this with alarm_settings[alarmi].alarm_setting_id with the output from notifListAlarmSettings.
                rc = notifDeleteAlarmSetting(alarm_setting_id);
                printf("notifDeleteAlarmSetting(): 0x%x\n", rc);
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    eventClose(&alarmevent);
    notifExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
