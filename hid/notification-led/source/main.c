// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use the HOME-button notification-LED, see also libnx hidsys.h.

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
    size_t i;
    size_t total_entries;
    u64 UniquePadIds[2];
    HidsysNotificationLedPattern pattern;

    printf("notification-led example\n");

    rc = hidsysInitialize();
    if (R_FAILED(rc)) {
        printf("hidsysInitialize(): 0x%x\n", rc);
        printf("Init failed, press + to exit.\n");
    }
    else {
        initflag = 1;
		printf("Press X for flashing notification-LED pattern.\n");
		printf("Press Y for faster strobe-like flashing notification-LED pattern.\n");
        printf("Press A to set a Breathing effect notification-LED pattern.\n");
        printf("Press B to set a Heartbeat effect notification-LED pattern.\n");
		printf("Press - to disable notification-LED only\n");
        printf("Press + to disable notification-LED and exit.\n");
    }

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) {
            // Disable notification led.
            memset(&pattern, 0, sizeof(pattern));
        }
		
		//only turn off all leds! 
		else if (kDown & KEY_MINUS) {
            // Disable notification led.
            memset(&pattern, 0, sizeof(pattern));
        }
		
		//my testing code 
		else if (kDown & KEY_X) {
            memset(&pattern, 0, sizeof(pattern));

            //Ely's code for flashing led on the home button on the joycons//    
            pattern.baseMiniCycleDuration = 0x1;             // 12.5 ms
            pattern.totalMiniCycles = 0x2;                   // 3 mini cycles. Last one 12.5ms.
            pattern.totalFullCycles = 0x0;                   // Repeat forever.
            pattern.startIntensity = 0xF;                    // 100%.


            pattern.miniCycles[0].ledIntensity = 0xF;        // 100%.
            pattern.miniCycles[0].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[0].finalStepDuration = 0x0;   // Forced 12.5ms.
            pattern.miniCycles[1].ledIntensity = 0x0;        // 0%.
            pattern.miniCycles[1].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[1].finalStepDuration = 0x0;   // Forced 12.5ms.

            pattern.miniCycles[2].ledIntensity = 0xF; 		  // 100%
            pattern.miniCycles[2].transitionSteps = 0xF;	  // 15 steps. Total 187.5ms.
            pattern.miniCycles[2].finalStepDuration = 0x0;	  // Forced 12.5ms.
            pattern.miniCycles[3].ledIntensity = 0x0; 		  // 0%
            pattern.miniCycles[3].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[3].finalStepDuration = 0x0;   // Forced 12.5ms.

			
			
        }
		
		
		else if (kDown & KEY_Y) {
            memset(&pattern, 0, sizeof(pattern));

            //Ely's test code for very fast strobe-like flashing led on the home button on the joycons//    
			//it is strobe-like!!!  :) - very sweet, I love this pattern!  :)  ELY M.   
            
			pattern.baseMiniCycleDuration = 0x1;             // 12.5 ms
            pattern.totalMiniCycles = 0x2;                   // 3 mini cycles. Last one 12.5ms.
            pattern.totalFullCycles = 0x0;                   // Repeat forever.
            pattern.startIntensity = 0xF;                    // 100%.
			
            pattern.miniCycles[0].ledIntensity = 0xF;        // 100%.
            pattern.miniCycles[0].transitionSteps = 0x0;     // Forced 12.5ms.
            pattern.miniCycles[0].finalStepDuration = 0x0;   // Forced 12.5ms.
            pattern.miniCycles[1].ledIntensity = 0x0;        // 0%.
            pattern.miniCycles[1].transitionSteps = 0x0;     // Forced 12.5ms.
            pattern.miniCycles[1].finalStepDuration = 0x0;   // Forced 12.5ms.

            pattern.miniCycles[2].ledIntensity = 0xF; 		  // 100%
            pattern.miniCycles[2].transitionSteps = 0x0;	  // Forced 12.5ms.
            pattern.miniCycles[2].finalStepDuration = 0x0;	  // Forced 12.5ms.
            pattern.miniCycles[3].ledIntensity = 0x0; 		  // 0%
            pattern.miniCycles[3].transitionSteps = 0x0;     // Forced 12.5ms.
            pattern.miniCycles[3].finalStepDuration = 0x0;   // Forced 12.5ms.

			
			
        }
		
        else if (kDown & KEY_A) {
            memset(&pattern, 0, sizeof(pattern));

            // Setup Breathing effect pattern data.
            pattern.baseMiniCycleDuration = 0x8;             // 100ms.
            pattern.totalMiniCycles = 0x2;                   // 3 mini cycles. Last one 12.5ms.
            pattern.totalFullCycles = 0x0;                   // Repeat forever.
            pattern.startIntensity = 0x2;                    // 13%.

            pattern.miniCycles[0].ledIntensity = 0xF;        // 100%.
            pattern.miniCycles[0].transitionSteps = 0xF;     // 15 steps. Transition time 1.5s.
            pattern.miniCycles[0].finalStepDuration = 0x0;   // Forced 12.5ms.
            pattern.miniCycles[1].ledIntensity = 0x2;        // 13%.
            pattern.miniCycles[1].transitionSteps = 0xF;     // 15 steps. Transition time 1.5s.
            pattern.miniCycles[1].finalStepDuration = 0x0;   // Forced 12.5ms. 
        }
        else if (kDown & KEY_B) {
            memset(&pattern, 0, sizeof(pattern));

            // Setup Heartbeat effect pattern data.
            pattern.baseMiniCycleDuration = 0x1;             // 12.5ms.
            pattern.totalMiniCycles = 0xF;                   // 16 mini cycles.
            pattern.totalFullCycles = 0x0;                   // Repeat forever.
            pattern.startIntensity = 0x0;                    // 0%.

            // First beat.
            pattern.miniCycles[0].ledIntensity = 0xF;        // 100%.
            pattern.miniCycles[0].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[0].finalStepDuration = 0x0;   // Forced 12.5ms.
            pattern.miniCycles[1].ledIntensity = 0x0;        // 0%.
            pattern.miniCycles[1].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
            pattern.miniCycles[1].finalStepDuration = 0x0;   // Forced 12.5ms.

            // Second beat.
            pattern.miniCycles[2].ledIntensity = 0xF;
            pattern.miniCycles[2].transitionSteps = 0xF;
            pattern.miniCycles[2].finalStepDuration = 0x0;
            pattern.miniCycles[3].ledIntensity = 0x0;
            pattern.miniCycles[3].transitionSteps = 0xF;
            pattern.miniCycles[3].finalStepDuration = 0x0;

            // Led off wait time.
            for(i=2; i<15; i++) {
                pattern.miniCycles[i].ledIntensity = 0x0;        // 0%.
                pattern.miniCycles[i].transitionSteps = 0xF;     // 15 steps. Total 187.5ms.
                pattern.miniCycles[i].finalStepDuration = 0xF;   // 187.5ms.
            }
        }

        if (kDown & (KEY_X | KEY_Y | KEY_A | KEY_B | KEY_MINUS | KEY_PLUS)) {
            total_entries = 0;
            memset(UniquePadIds, 0, sizeof(UniquePadIds));

            // Get the UniquePadIds for the specified controller, which will then be used with hidsysSetNotificationLedPattern.
            // If you want to get the UniquePadIds for all controllers, you can use hidsysGetUniquePadIds instead.
            rc = hidsysGetUniquePadsFromNpad(hidGetHandheldMode() ? CONTROLLER_HANDHELD : CONTROLLER_PLAYER_1, UniquePadIds, 2, &total_entries);
            printf("hidsysGetUniquePadsFromNpad(): 0x%x", rc);
            if (R_SUCCEEDED(rc)) printf(", %ld", total_entries);
            printf("\n");

            if (R_SUCCEEDED(rc)) {
                for(i=0; i<total_entries; i++) { // System will skip sending the subcommand to controllers where this isn't available.
                    printf("[%ld] = 0x%lx ", i, UniquePadIds[i]);
                    rc = hidsysSetNotificationLedPattern(&pattern, UniquePadIds[i]);
                    printf("hidsysSetNotificationLedPattern(): 0x%x\n", rc);
                }
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu
    }

    if (initflag) hidsysExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
