#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <switch.h>

//Example for usbds, see libnx usb.h. Switch-as-device<>host USB comms.
//Linux detects this as a serial device.
//See libnx usb_comms.c/usb.h for getting actual-transfer-size after using usbDsEndpoint_PostBufferAsync.

Result usbds_test(u8 *tmpbuf)
{
    Result ret=0;
    s32 tmpindex=0;
    UsbDsInterface* interface = NULL;
    UsbDsEndpoint *endpoint_in = NULL, *endpoint_out = NULL;

    struct usb_interface_descriptor interface_descriptor = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = USBDS_DEFAULT_InterfaceNumber,
                .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
                .bInterfaceSubClass = USB_CLASS_VENDOR_SPEC,
                .bInterfaceProtocol = USB_CLASS_VENDOR_SPEC,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_in = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_IN,
                .bmAttributes = USB_TRANSFER_TYPE_BULK,
                .wMaxPacketSize = 0x200,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_out = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_OUT,
                .bmAttributes = USB_TRANSFER_TYPE_BULK,
                .wMaxPacketSize = 0x200,
    };

    //Setup interface.
        ret = usbDsGetDsInterface(&interface, &interface_descriptor, "usb");
    if(R_FAILED(ret))return ret;

    //Setup endpoints.
    ret = usbDsInterface_GetDsEndpoint(interface, &endpoint_in, &endpoint_descriptor_in);//device->host
    if(R_FAILED(ret))return ret;

    ret = usbDsInterface_GetDsEndpoint(interface, &endpoint_out, &endpoint_descriptor_out);//host->device
    if(R_FAILED(ret))return ret;

    ret = usbDsInterface_EnableInterface(interface);
    if(R_FAILED(ret))return ret;

    //Wait for initialization to finish where data-transfer is usable. This includes waiting for the usb cable to be inserted if it's not already.
    ret = usbDsWaitReady();
    if(R_FAILED(ret))return ret;

    u32 somepos;
    for(somepos=0; somepos<0x101; somepos++)
    {
    memset(tmpbuf, 0, 0x1000);
    char *strptr = "\n";//"Hello World!\n";

    tmpbuf[0] = 0x11;
    tmpbuf[1] = 0x1;
    if(somepos==0 || somepos==0x101)
    {
        strncpy((char*)&tmpbuf[2], strptr, 0x1000-2);
        ret = usbDsEndpoint_PostBufferAsync(endpoint_in, tmpbuf, 2+strlen(strptr), NULL);
    }
    else
    {
        tmpbuf[2] = somepos-1;
        ret = usbDsEndpoint_PostBufferAsync(endpoint_in, tmpbuf, 2+1, NULL);
    }

    //Start a device->host transfer.
    //ret = usbDsEndpoint_PostBufferAsync(endpoint_in, tmpbuf, 2+1, NULL);
    //ret = usbDsEndpoint_PostBufferAsync(endpoint_in, tmpbuf, 2+strlen(strptr), NULL);
    if(R_FAILED(ret))return ret;
    //Wait for the transfer to finish.
    svcWaitSynchronization(&tmpindex, &endpoint_in->CompletionEvent, 1, U64_MAX);
    svcClearEvent(endpoint_in->CompletionEvent);
    }

    //Start a host->device transfer.
    ret = usbDsEndpoint_PostBufferAsync(endpoint_out, tmpbuf, 0x200, NULL);
    if(R_FAILED(ret))return ret;

    //Wait for the transfer to finish.
    svcWaitSynchronization(&tmpindex, &endpoint_out->CompletionEvent, 1, U64_MAX);
    svcClearEvent(endpoint_out->CompletionEvent);

    memcpy(&tmpbuf[0x400], tmpbuf, 0x200-2);
    tmpbuf[0] = 0x11;
    tmpbuf[1] = 0x1;
    memcpy(&tmpbuf[2], &tmpbuf[0x400], 0x200-2);

    //Start a device->host transfer.
    ret = usbDsEndpoint_PostBufferAsync(endpoint_in, tmpbuf, 0x2+1, NULL);
    if(R_FAILED(ret))
        return ret;

    //Wait for the transfer to finish.
    svcWaitSynchronization(&tmpindex, &endpoint_in->CompletionEvent, 1, U64_MAX);
    svcClearEvent(endpoint_in->CompletionEvent);

    svcSleepThread(5000000000);//Delay 5s

    return 0;
}

int main(int argc, char **argv)
{
    Result ret;

    UsbDsDeviceInfo deviceinfo = {
        .idVendor = 0x0403, // "Future Technology Devices International, Ltd"
        .idProduct = 0x6001, // "FT232 USB-Serial (UART) IC"
        .bcdDevice = 0x0200,
        .Manufacturer = "libnx",
        .Product = "usbds-example",
        .SerialNumber = "1337",
    };

    ret = usbDsInitialize(UsbComplexId_Default, &deviceinfo);

    if (R_SUCCEEDED(ret)) {
        u8 *tmpbuf = memalign(0x1000, 0x1000);//The buffer for PostBufferAsync commands must be 0x1000-byte aligned.
        if(tmpbuf==NULL)ret = -4;

        if (R_SUCCEEDED(ret)) ret = usbds_test(tmpbuf);

        usbDsExit();
    }

    if(R_FAILED(ret))
        fatalSimple(ret);

    svcSleepThread(5000000000);//Delay 5s

    return 0;
}

