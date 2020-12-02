// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use USB devices via usbhs, see also libnx usbhs.h/usb.h.
// Only devices which are not used by sysmodules are usable. This example will only detect/use USB mass storage devices.

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0, rc2=0;
    s32 total_entries=0;
    s32 i, epi, tmpi;
    Event inf_event;
    struct usb_endpoint_descriptor *ep_desc = NULL;
    UsbHsClientIfSession inf_session; // This example only uses 1 interface and 2 endpoints, an actual app may differ.
    UsbHsClientEpSession ep_sessions[2];
    UsbHsInterfaceFilter filter;
    UsbHsInterface interfaces[8];
    bool endpoints_found[2];

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

    printf("usbhs example\n");

    memset(&inf_event, 0, sizeof(inf_event));
    memset(&filter, 0, sizeof(filter));
    memset(interfaces, 0, sizeof(interfaces));
    memset(&inf_session, 0, sizeof(inf_session));
    memset(&ep_sessions, 0, sizeof(ep_sessions));

    // See libnx usbhs.h regarding filtering. Flags has to be set, since [7.0.0+] doesn't allow using a filter struct which matches an existing one.
    filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass;
    filter.bInterfaceClass = USB_CLASS_MASS_STORAGE;

    rc = usbHsInitialize();
    if (R_FAILED(rc)) printf("usbHsInitialize() failed: 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        rc = usbHsCreateInterfaceAvailableEvent(&inf_event, true, 0, &filter);
       if (R_FAILED(rc)) printf("usbHsCreateInterfaceAvailableEvent() failed: 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) printf("Ready.\n");

    rc2=rc;

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

        if (R_SUCCEEDED(rc2)) rc = eventWait(&inf_event, 0);
        if (R_SUCCEEDED(rc2) && R_SUCCEEDED(rc)) {
            printf("The available interfaces have changed.\n");

            memset(interfaces, 0, sizeof(interfaces));
            rc = usbHsQueryAvailableInterfaces(&filter, interfaces, sizeof(interfaces), &total_entries);
            if (R_FAILED(rc)) printf("usbHsQueryAvailableInterfaces() failed: 0x%x\n", rc);

            if (R_SUCCEEDED(rc)) {
                printf("total: %d\n", total_entries);
                for(i=0; i<total_entries; i++) {
                    printf("%d: pathstr = %s\n", i, interfaces[i].pathstr);//Run any interfaces[i] processing here.

                    // You can verify whether this is an interface you want to use by checking the content of interfaces[i], or do so later via control transfers etc.

                    rc = usbHsAcquireUsbIf(&inf_session, &interfaces[i]);
                    printf("usbHsAcquireUsbIf(interface index = %d) returned: 0x%x\n", i, rc);
                    if (R_SUCCEEDED(rc)) {
                        u8* tmpbuf = memalign(0x1000, 0x1000);// The allocated buffer address/size must be 0x1000-byte aligned, for usbHsIfCtrlXfer/usbHsEpPostBuffer.

                        if (tmpbuf) {
                            u32 transferredSize=0;

                            // Note that an actual app may want to validate that transferredSize==<input size>.

                            // This shows how to use a control transfer, an actual app might not need this.
                            // This gets descriptors starting with the config descriptor, via a control transfer.
                            memset(tmpbuf, 0, 0x1000);
                            rc = usbHsIfCtrlXfer(&inf_session, USB_ENDPOINT_IN, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_CONFIG<<8) | 0, 0, 0x40, tmpbuf, &transferredSize);
                            printf("usbHsIfCtrlXfer(interface index = %d) returned: 0x%x, transferredSize=0x%x\n", i, rc, transferredSize);
                            if (R_SUCCEEDED(rc)) {
                                for(tmpi=0; tmpi<transferredSize; tmpi++)printf("%02X", tmpbuf[tmpi]);
                                printf("\n");
                            }

                            rc = 1;

                            // Locate any endpoints you want to use by going through input_endpoint_descs/output_endpoint_descs.
                            // Note that official sw uses control transfer(s) to get the descriptors (see above).
                            // This example uses 1 OUTPUT and INPUT endpoint, with an actual app this may differ.
                            memset(endpoints_found, 0, sizeof(endpoints_found));
                            for(epi=0; epi<15; epi++) {
                                ep_desc = &inf_session.inf.inf.output_endpoint_descs[epi];
                                if (ep_desc->bLength != 0 && ep_desc->bEndpointAddress == (USB_ENDPOINT_OUT | 0x1)) {
                                    printf("Using OUTPUT endpoint %d.\n", epi);

                                    endpoints_found[0] = true;
                                    rc = usbHsIfOpenUsbEp(&inf_session, &ep_sessions[0], 1, ep_desc->wMaxPacketSize, ep_desc);
                                    printf("usbHsIfOpenUsbEp returned: 0x%x\n", rc);
                                    if (R_FAILED(rc)) break;
                                }

                                ep_desc = &inf_session.inf.inf.input_endpoint_descs[epi];
                                if (ep_desc->bLength != 0 && ep_desc->bEndpointAddress == (USB_ENDPOINT_IN | 0x2)) {
                                    printf("Using INPUT endpoint %d.\n", epi);

                                    endpoints_found[1] = true;
                                    rc = usbHsIfOpenUsbEp(&inf_session, &ep_sessions[1], 1, ep_desc->wMaxPacketSize, ep_desc);
                                    printf("usbHsIfOpenUsbEp returned: 0x%x\n", rc);
                                    if (R_FAILED(rc)) break;
                                }

                                if (endpoints_found[0] && endpoints_found[1]) break;
                            }

                            if (!endpoints_found[0] || !endpoints_found[1]) {
                                printf("Failed to find the required endpoints.\n");
                                rc = 2;
                            }

                            // Use the endpoints. An actual app would have different endpoint handling, depending on the USB device.

                            // Write data to the OUTPUT endpoint. In this case this is a mass-storage SCSI Inquiry.
                            // Note that an actual app may want to validate transferredSize.
                            if (R_SUCCEEDED(rc)) {
                                memset(tmpbuf, 0, 0x1000);

                                u8 tmpdata[] = {0x55, 0x53, 0x42, 0x43, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x12, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                                memcpy(tmpbuf, tmpdata, sizeof(tmpdata));

                                transferredSize = 0;
                                rc = usbHsEpPostBuffer(&ep_sessions[0], tmpbuf, sizeof(tmpdata), &transferredSize);
                                printf("usbHsEpPostBuffer returned: 0x%x, transferredSize=0x%x\n", rc, transferredSize);
                            }

                            // Read data from the INPUT endpoint. In this case this is mass-storage SCSI Inquiry response data.
                            // Note that an actual app may want to validate transferredSize.
                            if (R_SUCCEEDED(rc)) {
                                memset(tmpbuf, 0, 0x1000);

                                transferredSize = 0;
                                rc = usbHsEpPostBuffer(&ep_sessions[1], tmpbuf, 0x200, &transferredSize);
                                printf("usbHsEpPostBuffer returned: 0x%x, transferredSize=0x%x\n", rc, transferredSize);
                                if (R_SUCCEEDED(rc)) {
                                    for(tmpi=0; tmpi<transferredSize; tmpi++)printf("%02X", tmpbuf[tmpi]);
                                    printf("\n");
                                }
                            }

                            // At this point this example is done using the endpoints, close these.
                            usbHsEpClose(&ep_sessions[0]);
                            usbHsEpClose(&ep_sessions[1]);

                            free(tmpbuf);
                        }
                        else
                            printf("tmpbuf alloc failed.\n");

                        usbHsIfClose(&inf_session); // At this point this example is done using the interface, close it.
                    }
                }
            }
        }

        // Signaled when a device was removed, cleanup state if our interface can't be found in the usbHsQueryAcquiredInterfaces output.
        if (R_SUCCEEDED(rc2)) rc = eventWait(usbHsGetInterfaceStateChangeEvent(), 0);
        if (R_SUCCEEDED(rc2) && R_SUCCEEDED(rc)) {
            eventClear(usbHsGetInterfaceStateChangeEvent());

            printf("InterfaceStateChangeEvent was signaled.\n");

            // Only need to check the rest of this if our interface is actually initialized. This won't actually run since this example already closed the interface above.
            if (usbHsIfIsActive(&inf_session)) {
                memset(interfaces, 0, sizeof(interfaces));
                rc = usbHsQueryAcquiredInterfaces(interfaces, sizeof(interfaces), &total_entries);
                if (R_FAILED(rc)) printf("usbHsQueryAcquiredInterfaces() failed: 0x%x\n", rc);

                if (R_SUCCEEDED(rc)) {
                    bool found_flag = 0;
                    for(i=0; i<total_entries; i++) {
                        if (usbHsIfGetID(&inf_session) == interfaces[i].inf.ID) {
                            found_flag = 1;
                            break;
                        }
                    }

                    if(!found_flag) {
                        printf("Interface not found, cleaning up state...\n");
                        usbHsEpClose(&ep_sessions[0]);
                        usbHsEpClose(&ep_sessions[1]);
                        usbHsIfClose(&inf_session);
                    }
                }
            }
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Make sure to cleanup interface/endpoint state before exiting, if it's not already closed.

    usbHsDestroyInterfaceAvailableEvent(&inf_event, 0);
    usbHsExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
