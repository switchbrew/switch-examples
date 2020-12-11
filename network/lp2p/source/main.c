// Include the most common headers from the C standard library
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <unistd.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use lp2p, for Switch-hosted local-network communications with mainly non-Switch devices. See also libnx lp2p.h.

// Note that in an actual app you may want to use lp2p/sockets from another thread.

Result create_network(const Lp2pGroupInfo *in_group_info) {
    Result rc=0;

    rc = lp2pCreateGroup(in_group_info);
    printf("lp2pCreateGroup(): 0x%x\n", rc);

    return rc;
}

void leave_network(void) {
    Result rc=0;
    u32 tmp32=0;

    rc = lp2pLeave(&tmp32);
    printf("lp2pLeave(): 0x%x, 0x%x\n", rc, tmp32);

    rc = lp2pDestroyGroup();
    printf("lp2pDestroyGroup(): 0x%x\n", rc);
}

void generate_random_str(char *str, size_t string_len) {
    if (string_len & 1) string_len--;
    for (size_t i=0; i<string_len; i+=2) {
        u8 tmp=0;
        randomGet(&tmp, sizeof(tmp));
        snprintf(&str[i], 3, "%02X", tmp);
    }
}

int main(int argc, char **argv)
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

    // Initialise sockets
    socketInitializeDefault();

    printf("lp2p example\n");

    int sockfd=-1;
    struct sockaddr_in serv_addr;

    Result rc=0;
    Lp2pIpConfig ip_config={0};
    Lp2pGroupInfo in_group_info={0};
    Lp2pGroupInfo group_info={0};

    lp2pCreateGroupInfo(&in_group_info);

    // This example uses [11.0.0+] WPA2-PSK. If you want to use Nintendo-encryption for the network (for communicating with accessories such as mklive), use flags=1 (which is also the default), and replace the lp2pGroupInfoSetPassphrase call with lp2pGroupInfoSetPresharedKey.
    // If you want to use plaintext (open network), set in_group_info.security_type manually, and call lp2pGroupInfoSetPresharedKey (where the used keydata doesn't matter).

    s8 flags=0;
    lp2pGroupInfoSetFlags(&in_group_info, &flags, 1);

    // See lp2p.h Lp2pGroupInfo::service_name.

    char ssid[0x20] = {"libnxlp2p-"};
    char passphrase[0x40] = {0};
    generate_random_str(&ssid[strlen(ssid)], 8);
    generate_random_str(passphrase, 16);

    lp2pGroupInfoSetServiceName(&in_group_info, ssid);
    lp2pGroupInfoSetPassphrase(&in_group_info, passphrase);

    rc = lp2pInitialize(Lp2pServiceType_App);
    printf("lp2pInitialize(): 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        printf("Press A to create a network.\n");
        printf("Press X to leave a network.\n");
        printf("Press the D-Pad buttons while on a network to send messages.\n");
    }
    printf("Press + to exit.\n");

    // Main loop
    while(appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u32 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        if (R_SUCCEEDED(rc) && (kDown & HidNpadButton_A)) {
            Result rc2 = create_network(&in_group_info);
            printf("create_network(): 0x%x\n", rc2);

            // We didn't set the full ServiceName, so the sysmodule will generate the rest of it. Use lp2pGetGroupInfo to get the actual ServiceName.
            if (R_SUCCEEDED(rc2)) {
                rc2 = lp2pGetGroupInfo(&group_info);
                printf("lp2pGetGroupInfo(): 0x%x\n", rc2);
            }

            if (R_SUCCEEDED(rc2)) {
                rc2 = lp2pGetIpConfig(&ip_config);
                printf("lp2pGetIpConfig(): 0x%x\n", rc2);
            }

            if (R_SUCCEEDED(rc2)) {
                printf("Network created.\nSSID: %s\nPassphrase: %s\n", group_info.service_name, passphrase);
            }

            // Once on a network, you can use whatever sockets you want. In this example, we'll send/recv data with UDP-broadcast.

            if (R_SUCCEEDED(rc2)) {
                sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                if (sockfd < 0) {
                    printf("socket() failed\n");
                }
                else {
                    int ret=0;

                    int optval = 1;
                    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval));
                    if (ret==-1) {
                        printf("setsockopt() failed\n");
                        close(sockfd);
                        sockfd = -1;
                    }

                    // Setup a broadcast addr. If you want to use IP addrs for members on the network, see lp2pGetMembers().
                    if (sockfd >= 0) {
                        memset(&serv_addr, 0, sizeof(serv_addr));
                        serv_addr.sin_family = AF_INET;
                        serv_addr.sin_addr.s_addr = htonl(ntohl(((struct sockaddr_in*)ip_config.ip_addr)->sin_addr.s_addr) | ~ntohl(((struct sockaddr_in*)ip_config.subnet_mask)->sin_addr.s_addr));
                        serv_addr.sin_port = htons(7777);

                        if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
                            printf("bind failed\n");
                            close(sockfd);
                            sockfd = -1;
                        }
                    }
                }
            }
        }

        if (R_SUCCEEDED(rc) && (kDown & HidNpadButton_X)) {
            if (sockfd >= 0) {
                close(sockfd);
                sockfd = -1;
            }
            leave_network();
        }

        if (R_SUCCEEDED(rc) && sockfd>=0 && (kDown & (HidNpadButton_Left|HidNpadButton_Right|HidNpadButton_Up|HidNpadButton_Down))) {
            char tmpstr[32];
            memset(tmpstr, 0, sizeof(tmpstr));

            if (kDown & HidNpadButton_Left) strncpy(tmpstr, "Button Left pressed.", sizeof(tmpstr)-1);
            else if (kDown & HidNpadButton_Right) strncpy(tmpstr, "Button Right pressed.", sizeof(tmpstr)-1);
            else if (kDown & HidNpadButton_Up) strncpy(tmpstr, "Button Up pressed.", sizeof(tmpstr)-1);
            else if (kDown & HidNpadButton_Down) strncpy(tmpstr, "Button Down pressed.", sizeof(tmpstr)-1);

            ssize_t ret = sendto(sockfd, tmpstr, sizeof(tmpstr), 0, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in));
            int tmp = errno;
            printf("sendto(): %ld", ret);
            if (ret < 0) printf(", %s", strerror(tmp));
            printf("\n");
        }

        if (R_SUCCEEDED(rc) && sockfd>=0) {
            char tmpstr[32];
            memset(tmpstr, 0, sizeof(tmpstr));

            struct sockaddr_in src_addr={0};
            socklen_t fromlen = sizeof(struct sockaddr_in);
            ssize_t ret = recvfrom(sockfd, tmpstr, sizeof(tmpstr), MSG_DONTWAIT, (struct sockaddr*) &src_addr, &fromlen);

            tmpstr[sizeof(tmpstr)-1] = 0;
            if (ret>0) printf("Received data: %s\n", tmpstr);
        }

        consoleUpdate(NULL);
    }

    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }

    leave_network();
    lp2pExit();
    socketExit();
    consoleExit(NULL);
    return 0;
}
