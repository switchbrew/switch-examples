// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use USB devices via usbhs, see also libnx usbhs.h/usb.h.
// Only devices which are not used by sysmodules are usable. With the below filter, this will only detect USB mass storage devices.

// Main program entrypoint
int main(int argc, char* argv[])
{
    Result rc=0;
    s32 total_entries=0;
    s32 i, epi, tmpi;
    Event inf_event;
    struct usb_endpoint_descriptor *ep_desc = NULL;
    UsbHsClientIfSession inf_session;//This example only uses 1 interface/endpoint, an actual app may use multiple.
    UsbHsClientEpSession ep_session;
    UsbHsInterfaceFilter filter;
    UsbHsInterface interfaces[8];

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    printf("usbhs example\n");

    memset(&inf_event, 0, sizeof(inf_event));
    memset(&filter, 0, sizeof(filter));
    memset(interfaces, 0, sizeof(interfaces));
    memset(&inf_session, 0, sizeof(inf_session));
    memset(&ep_session, 0, sizeof(ep_session));

    //See libnx usbhs.h regarding filtering. Flags has to be set, since [7.0.0+] doesn't allow using a filter struct which matches an existing one.
    filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass;
    filter.bInterfaceClass = USB_CLASS_MASS_STORAGE;

    rc = usbHsInitialize();
    if (R_FAILED(rc)) printf("usbHsInitialize() failed: 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        rc = usbHsCreateInterfaceAvailableEvent(&inf_event, true, 0, &filter);
       if (R_FAILED(rc)) printf("usbHsCreateInterfaceAvailableEvent() failed: 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) printf("Ready.\n");

    // Main loop
    while (appletMainLoop())
    {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        rc = eventWait(&inf_event, 0);
        if (R_SUCCEEDED(rc)) {
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
                            for(epi=0; epi<15; epi++) {
                                ep_desc = &inf_session.inf.inf.input_endpoint_descs[epi];
                                if(ep_desc->bLength != 0) {
                                    printf("Using endpoint %d.\n", epi);

                                    rc = usbHsIfOpenUsbEp(&inf_session, &ep_session, 1, ep_desc->wMaxPacketSize, ep_desc);
                                    printf("usbHsIfOpenUsbEp returned: 0x%x\n", rc);

                                    break;
                                }
                            }

                            // Since this example uses just any device, this may not work for some devices since this doesn't use a proper protocol. Some devices might need unplugged and plugged back in for this to work, after running this example once.
                            if (R_SUCCEEDED(rc)) {
                                memset(tmpbuf, 0, 0x1000);
                                transferredSize = 0;
                                rc = usbHsEpPostBuffer(&ep_session, tmpbuf, 0x10, &transferredSize);
                                printf("usbHsEpPostBuffer returned: 0x%x, transferredSize=0x%x\n", rc, transferredSize);
                                if (R_SUCCEEDED(rc)) {
                                    for(tmpi=0; tmpi<transferredSize; tmpi++)printf("%02X", tmpbuf[tmpi]);
                                    printf("\n");
                                }

                                usbHsEpClose(&ep_session);//At this point this example is done using the endpoint, close it.
                            }

                            free(tmpbuf);
                        }
                        else
                            printf("tmpbuf alloc failed.\n");

                        usbHsIfClose(&inf_session);//At this point this example is done using the interface, close it.
                    }
                }
            }
        }

        // Signaled when a device was removed, cleanup state if our interface can't be found in the usbHsQueryAcquiredInterfaces output.
        rc = eventWait(usbHsGetInterfaceStateChangeEvent(), 0);
        if (R_SUCCEEDED(rc)) {
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
                        usbHsEpClose(&ep_session);
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
