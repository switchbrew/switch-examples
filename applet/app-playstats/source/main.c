// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use applet to get playstats for applications. See also libnx applet.h. See applet.h for the requirements for using this.
// This also shows how to use pdmqry, see also libnx pdm.h.

/// Main program entrypoint
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

    printf("application play-stats example\n");

    Result rc=0;
    PdmApplicationPlayStatistics stats[1];
    PdmAppletEvent events[1];
    u64 application_ids[1] = {0x010021B002EEA000}; // Change this to the ApplicationId of the current-process / the ApplicationId you want to use.
    s32 total_out;
    s32 i;
    bool initflag=0;

    // Only needed when using the cmds which require the Uid.
    AccountUid preselected_uid={0};
    rc = accountInitialize(AccountServiceType_Application);
    if (R_SUCCEEDED(rc)) {
        rc = accountGetPreselectedUser(&preselected_uid);
        accountExit();
    }
    if (R_FAILED(rc)) printf("Failed to get user: 0x%x\n", rc);

    // Not needed if you just want to use the applet cmds.
    if (R_SUCCEEDED(rc)) {
        rc = pdmqryInitialize();
        if (R_FAILED(rc)) printf("pdmqryInitialize(): 0x%x\n", rc);
        if (R_SUCCEEDED(rc)) initflag = true;
    }

    printf("Press A to get playstats.\n");
    if (initflag) printf("Press X to use pdmqry.\n");
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

        if (kDown & HidNpadButton_A) {
            // Use appletQueryApplicationPlayStatisticsByUid if you want playstats for a specific userID.

            memset(stats, 0, sizeof(stats));
            total_out = 0;

            if (R_SUCCEEDED(rc)) {
                rc = appletQueryApplicationPlayStatistics(stats, application_ids, sizeof(application_ids)/sizeof(u64), &total_out);
                printf("appletQueryApplicationPlayStatistics(): 0x%x\n", rc);
                //rc = appletQueryApplicationPlayStatisticsByUid(preselected_uid, stats, application_ids, sizeof(application_ids)/sizeof(u64), &total_out);
                //printf("appletQueryApplicationPlayStatisticsByUid(): 0x%x\n", rc);
            }
            if (R_SUCCEEDED(rc)) {
                printf("total_out: %d\n", total_out);
                for (i=0; i<total_out; i++) {
                    printf("%d: ", i);
                    printf("application_id = 0x%08lX, playtime = %lu (%lu seconds), total_launches = %lu\n", stats[i].application_id, stats[i].playtime, stats[i].playtime / 1000000000UL, stats[i].total_launches);
                }
            }
        }

        if (initflag && (kDown & HidNpadButton_X)) {
            memset(events, 0, sizeof(events));
            total_out = 0;

            // Get PdmAppletEvents.
            rc = pdmqryQueryAppletEvent(false, 0, events, sizeof(events)/sizeof(PdmAppletEvent), &total_out);
            printf("pdmqryQueryAppletEvent(): 0x%x\n", rc);
            if (R_SUCCEEDED(rc)) {
                printf("total_out: %d\n", total_out);
                for (i=0; i<total_out; i++) {
                    time_t tmptime = events[i].timestamp_user;

                    printf("%d: ", i);
                    printf("program_id = 0x%08lX, entry_index = 0x%x, timestamp_user = %lu, timestamp_network = %lu, event_type = %u, timestamp_user = %s\n", events[i].program_id, events[i].entry_index, events[i].timestamp_user, events[i].timestamp_network, events[i].event_type, ctime(&tmptime));
                }
            }

            // Get PdmPlayStatistics for the specified ApplicationId.
            PdmPlayStatistics playstats[1]={0};
            rc = pdmqryQueryPlayStatisticsByApplicationId(application_ids[0], false, &playstats[0]);
            printf("pdmqryQueryPlayStatisticsByApplicationId(): 0x%x\n", rc);
            if (R_SUCCEEDED(rc)) printf("program_id = 0x%016lX, playtime = %lu (%lu seconds), total_launches = %u\n", playstats[0].program_id, playstats[0].playtime, playstats[0].playtime / 1000000000UL, playstats[0].total_launches);

            // Get PdmPlayStatistics for the specified ApplicationId and user.
            rc = pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(application_ids[0], preselected_uid, false, &playstats[0]);
            printf("pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(): 0x%x\n", rc);
            if (R_SUCCEEDED(rc)) printf("program_id = 0x%016lX, playtime = %lu (%lu seconds), total_launches = %u\n", playstats[0].program_id, playstats[0].playtime, playstats[0].playtime / 1000000000UL, playstats[0].total_launches);

            // Get a listing of PdmLastPlayTime for the specified applications.
            PdmLastPlayTime playtimes[1]={0};
            rc = pdmqryQueryLastPlayTime(false, playtimes, application_ids, 1, &total_out);
            printf("pdmqryQueryLastPlayTime(): 0x%x, %d\n", rc, total_out);
            if (R_SUCCEEDED(rc)) {
                for (i=0; i<total_out; i++)
                    printf("%d: timestamp_user = %lu\n", i, pdmPlayTimestampToPosix(playtimes[i].timestamp_user));
            }

            // Get the available range for reading events, see pdm.h.
            s32 total_entries=0, start_entryindex=0, end_entryindex=0;
            rc = pdmqryGetAvailablePlayEventRange(&total_entries, &start_entryindex, &end_entryindex);
            printf("pdmqryGetAvailablePlayEventRange(): 0x%x, 0x%x, 0x%x, 0x%x\n", rc, total_entries, start_entryindex, end_entryindex);

            // Get account events.
            PdmAccountEvent accevents[5]={};
            rc = pdmqryQueryAccountEvent(0, accevents, 5, &total_out);
            printf("pdmqryQueryAccountEvent(): 0x%x\n", rc);
            if (R_SUCCEEDED(rc)) {
                printf("total_out: %d\n", total_out);
                for (i=0; i<total_out; i++) {
                    time_t tmptime = accevents[i].timestamp_user;
                    printf("%d: ", i);
                    printf("uid = 0x%lx 0x%lx, program_id = 0x%016lX, entry_index = 0x%x, timestamp_user = %lu, timestamp_network = %lu, event_type = %u, timestamp_user = %s\n", accevents[i].uid.uid[0], accevents[i].uid.uid[1], accevents[i].program_id, accevents[i].entry_index, accevents[i].timestamp_user, accevents[i].timestamp_network, accevents[i].type, ctime(&tmptime));
                }
            }

            // For more cmds, see pdm.h.
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    pdmqryExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
