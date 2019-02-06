// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// See also libnx nfc.h.

// Indefinitely wait for an event to be signaled
// Break when + is pressed, or if the application should quit (in this case, return value will be non-zero)
Result eventWaitLoop(Event *event) {
    Result rc = 1;
    while (appletMainLoop()) {
        rc = eventWait(event, 0);
        hidScanInput();
        if (R_SUCCEEDED(rc) || (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS))
            break;
    }
    return rc;
}

// Print raw data as hexadecimal numbers.
void print_hex(void *buf, size_t size) {
    u8 *data = (u8 *)buf;
    for (size_t i=0; i<size; i++)
        printf("%02X", data[i]);
    printf("\n");
    consoleUpdate(NULL);
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc = 0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("NFC example program.\n");
    printf("Scan an amiibo tag to display its character.\n");
    printf("Press + to exit.\n\n");
    consoleUpdate(NULL);

    // Initialize the nfp:user and nfc:user services.
    rc = nfpuInitialize();

    // Check if NFC is enabled. If not, wait until it is.
    if (R_SUCCEEDED(rc)) {
        bool nfc_enabled = false;
        rc = nfpuIsNfcEnabled(&nfc_enabled);

        if (R_SUCCEEDED(rc) && !nfc_enabled) {
            // Get the availability change event. This is signaled when a change in NFC availability happens.
            Event availability_change_event = {0};
            rc = nfpuAttachAvailabilityChangeEvent(&availability_change_event);

            // Wait for a change in availability.
            if (R_SUCCEEDED(rc)) {
                printf("NFC is disabled. Please turn off plane mode via the quick settings to continue.\n");
                consoleUpdate(NULL);
                rc = eventWaitLoop(&availability_change_event);
            }

            eventClose(&availability_change_event);
        }
    }

    // Get the handle of the first controller with NFC capabilities.
    HidControllerID controller = 0;
    if (R_SUCCEEDED(rc)) {
        u32 device_count;
        rc = nfpuListDevices(&device_count, &controller, 1);
    }

    if (R_FAILED(rc))
        goto fail_0;

    // Get the activation event. This is signaled when a tag is detected.
    Event activate_event = {0};
    if (R_FAILED(nfpuAttachActivateEvent(controller, &activate_event)))
        goto fail_1;

    // Get the deactivation event. This is signaled when a tag is removed.
    Event deactivate_event = {0};
    if (R_FAILED(nfpuAttachDeactivateEvent(controller, &deactivate_event)))
        goto fail_2;

    // Start the detection of tags.
    rc = nfpuStartDetection(controller);
    if (R_SUCCEEDED(rc)) {
        printf("Scanning for a tag...\n");
        consoleUpdate(NULL);
    }

    // Wait until a tag is detected.
    if (R_SUCCEEDED(rc)) {
        rc = eventWaitLoop(&activate_event);
        if (R_SUCCEEDED(rc)) {
            printf("A tag was detected, please do not remove it from the NFC spot.\n");
            consoleUpdate(NULL);
        }
    }

    // If a tag was successfully detected, load it into memory.
    if (R_SUCCEEDED(rc))
        rc = nfpuMount(controller, NfpuDeviceType_Amiibo, NfpuMountTarget_All);

    // Retrieve the model info data, which contains the amiibo id.
    if (R_SUCCEEDED(rc)) {
        NfpuModelInfo model_info = {0};
        rc = nfpuGetModelInfo(controller, &model_info);
        
        if (R_SUCCEEDED(rc)) {
            printf("Amiibo ID: ");
            print_hex(model_info.amiibo_id, 8);
            consoleUpdate(NULL);
        }
    }
    
    if (R_SUCCEEDED(rc)) {
        printf("You can now remove the tag.\n");
        consoleUpdate(NULL);
        eventWaitLoop(&deactivate_event);
    }

    // If an error happened during detection/reading, print it.
    if (R_FAILED(rc))
        printf("Error: 0x%x.\n", rc);
    
    // Unmount the tag.
    nfpuUnmount(controller);

    // Stop the detection of tags.
    nfpuStopDetection(controller);

    // Wait for the user to explicitely exit.
    printf("Press + to exit.\n");
    while (appletMainLoop()) {
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
            break;

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Cleanup.
fail_2: 
    eventClose(&deactivate_event);
fail_1:
    eventClose(&activate_event);
fail_0:
    nfpuExit();
    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
