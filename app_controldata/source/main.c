#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <switch.h>

//This example shows how to get NsApplicationControlData for a title, which contains nacp/icon. See libnx ns.h.

int main(int argc, char **argv)
{
    Result rc=0;

    u64 titleID=0x01007ef00011e000;//titleID for use with nsGetApplicationControlData, in this case BOTW.
    NsApplicationControlData *buf=NULL;
    size_t outsize=0;

    NacpLanguageEntry *langentry = NULL;
    char name[0x201];

    consoleInit(NULL);

    buf = (NsApplicationControlData*)malloc(sizeof(NsApplicationControlData));
    if (buf==NULL) {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        printf("Failed to alloc mem.\n");
    }
    else {
        memset(buf, 0, sizeof(NsApplicationControlData));
    }

    if (R_SUCCEEDED(rc)) {
        rc = nsInitialize();
        if (R_FAILED(rc)) {
            printf("nsInitialize() failed: 0x%x\n", rc);
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = nsGetApplicationControlData(1, titleID, buf, sizeof(NsApplicationControlData), &outsize);
        if (R_FAILED(rc)) {
            printf("nsGetApplicationControlData() failed: 0x%x\n", rc);
        }

        if (outsize < sizeof(buf->nacp)) {
            rc = -1;
            printf("Outsize is too small: 0x%lx.\n", outsize);
        }

        if (R_SUCCEEDED(rc)) {
            rc = nacpGetLanguageEntry(&buf->nacp, &langentry);

            if (R_FAILED(rc) || langentry==NULL) printf("Failed to load LanguageEntry.\n");
        }

        if (R_SUCCEEDED(rc)) {
            memset(name, 0, sizeof(name));
            strncpy(name, langentry->name, sizeof(name)-1);//Don't assume the nacp string is NUL-terminated for safety.

            printf("Name: %s\n", name);//Note that the print-console doesn't support UTF-8. The name is UTF-8, so this will only display properly if there isn't any non-ASCII characters. To display it properly, a print method which supports UTF-8 should be used instead.

            //You could also load the JPEG icon from buf->nacp.icon. The icon size can be calculated with: iconsize = outsize - sizeof(buf->nacp);
        }

        nsExit();
    }

    free(buf);

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
