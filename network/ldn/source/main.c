// Include the most common headers from the C standard library
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <unistd.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// This example shows how to use ldn, for local-network communications. See also libnx ldn.h.
// All systems in the network must be running the same hbl host Application. If any systems are running non-Application (where control.nacp loading fails), all systems must be running non-Application as well.

// Note that in an actual app you may want to use ldn/sockets from another thread.

// Replace this keydata with your own. See ldn.h SecurityConfig for the required size.
static const u8 sec_data[0x10]={0x04, 0xb9, 0x9d, 0x4d, 0x58, 0xbc, 0x65, 0xe1, 0x77, 0x13, 0xc2, 0xb8, 0xd1, 0xb8, 0xec, 0xf6};

Result create_network(const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, const LdnNetworkConfig *netconfig, const void* advert, size_t advert_size) {
    Result rc=0;

    rc = ldnOpenAccessPoint();
    printf("ldnOpenAccessPoint(): 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        rc = ldnCreateNetwork(sec_config, user_config, netconfig);
        printf("ldnCreateNetwork(): 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        rc = ldnSetAdvertiseData(advert, advert_size);
        printf("ldnSetAdvertiseData(): 0x%x\n", rc);
    }

    if (R_FAILED(rc)) ldnCloseAccessPoint();

    return rc;
}

Result connect_network(const LdnScanFilter *filter, const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, const void* advert, size_t advert_size) {
    Result rc=0;
    s32 total_out=0;
    LdnNetworkInfo netinfo_list[0x18];

    rc = ldnOpenStation();
    printf("ldnOpenStation(): 0x%x\n", rc);

    if (R_SUCCEEDED(rc)) {
        rc = ldnScan(0, filter, netinfo_list, 0x18, &total_out);
        printf("ldnScan(): 0x%x, %d\n", rc, total_out);
    }

    // In an actual app you'd display the output netinfo_list and let the user select which one to connect to, however in this example we'll just connect to the first one.

    if (R_SUCCEEDED(rc) && !total_out) {
        rc = MAKERESULT(Module_Libnx, LibnxError_NotFound);
        printf("No network(s) found.\n");
    }

    if (R_SUCCEEDED(rc)) { // Handle this / parse it with any method you want.
        if (netinfo_list[0].advertise_data_size!=advert_size || memcmp(netinfo_list[0].advertise_data, advert, advert_size)!=0) {
            rc = MAKERESULT(Module_Libnx, LibnxError_NotFound);
            printf("The found network advert data doesn't match.\n");
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = ldnConnect(sec_config, user_config, 0, 0, &netinfo_list[0]);
        printf("ldnConnect(): 0x%x\n", rc);
    }

    if (R_FAILED(rc)) ldnCloseStation();

    return rc;
}

void leave_network(void) {
    Result rc=0;
    LdnState state;

    rc = ldnGetState(&state);
    printf("ldnGetState(): 0x%x, %d\n", rc, state);

    if (R_SUCCEEDED(rc)) {
        if (state==LdnState_AccessPointOpened || state==LdnState_AccessPointCreated) {
            if (state==LdnState_AccessPointCreated) {
                rc = ldnDestroyNetwork();
                printf("ldnDestroyNetwork(): 0x%x\n", rc);
            }
            rc = ldnCloseAccessPoint();
            printf("ldnCloseAccessPoint(): 0x%x\n", rc);
        }

        if (state==LdnState_StationOpened || state==LdnState_StationConnected) {
            if (state==LdnState_StationConnected) {
                rc = ldnDisconnect();
                printf("ldnDisconnect(): 0x%x\n", rc);
            }
            rc = ldnCloseStation();
            printf("ldnCloseStation(): 0x%x\n", rc);
        }
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

    // Initialise sockets
    socketInitializeDefault();

    printf("ldn example\n");

    int sockfd=-1;
    struct sockaddr_in serv_addr;

    Result rc=0;
    LdnIpv4Address addr={0};
    LdnSubnetMask mask={0};
    LdnState state;
    LdnNetworkInfo netinfo={0};
    LdnSecurityConfig sec_config={0};
    LdnUserConfig user_config={0};
    LdnNetworkConfig netconfig={0};
    LdnScanFilter filter={0};

    // In this example we just set the AdvertiseData to this string, and compare it during scanning. This is done to verify that the network belongs to this app, you can handle this with any method you want.
    static const char advert[] = "libnx ldn example";

    // Load the preselected-user nickname into user_config - you can use anything for the nickname if you want.
    strncpy(user_config.nickname, "nickname", sizeof(user_config.nickname)); // Fallback nickname in case the below fails.
    rc = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(rc)) {
        printf("accountInitialize() failed: 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        AccountUid presel_uid={0};
        AccountProfile profile={0};
        AccountProfileBase profilebase={0};

        rc = accountGetPreselectedUser(&presel_uid);
        if (R_FAILED(rc)) {
            printf("accountGetPreselectedUser() failed: 0x%x\n", rc);
        }

        if (R_SUCCEEDED(rc)) {
            rc = accountGetProfile(&profile, presel_uid);

            if (R_FAILED(rc)) {
                printf("accountGetProfile() failed: 0x%x\n", rc);
            }
        }

        if (R_SUCCEEDED(rc)) {
            rc = accountProfileGet(&profile, NULL, &profilebase);

            if (R_FAILED(rc)) {
                printf("accountProfileGet() failed: 0x%x\n", rc);
            }

            if (R_SUCCEEDED(rc)) {
                strncpy(user_config.nickname, profilebase.nickname, sizeof(user_config.nickname));
                user_config.nickname[sizeof(user_config.nickname)-1] = 0;
            }
        }
        accountProfileClose(&profile);
        accountExit();
    }

    netconfig.local_communication_id = -1;
    netconfig.participant_max = 8; // Adjust as needed.
    netconfig.userdata_filter = 0x4248; // "HB", adjust this if you want.
    // For more netconfig fields, see ldn.h.
    // Set local_communication_version if you want to only allow devices on the network if this version value matches (and update the version passed to ldnConnect).

    sec_config.type = 1;
    sec_config.data_size = sizeof(sec_data);
    memcpy(sec_config.data, sec_data, sizeof(sec_data));

    filter.local_communication_id = -1;
    filter.userdata_filter = netconfig.userdata_filter;
    filter.flags = LdnScanFilterFlags_LocalCommunicationId | LdnScanFilterFlags_UserData;

    rc = ldnInitialize(LdnServiceType_User);
    printf("ldnInitialize(): 0x%x\n", rc);

    Event state_event={0};
    if (R_SUCCEEDED(rc)) {
        rc = ldnAttachStateChangeEvent(&state_event);
        printf("ldnAttachStateChangeEvent(): 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        printf("Press A to create a network.\n");
        printf("Press B to connect to a network.\n");
        printf("Press X to leave a network.\n");
        printf("Press the D-Pad buttons while on a network to send messages.\n");
    }
    printf("Press + to exit.\n");

    // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        if (R_SUCCEEDED(rc) && (kDown & (KEY_A|KEY_B))) {
             Result rc2 = ldnGetState(&state);
             printf("ldnGetState(): 0x%x, %d\n", rc, state);
             if (R_SUCCEEDED(rc2) && state==LdnState_Initialized) {
                 if (kDown & KEY_A) {
                     rc2 = create_network(&sec_config, &user_config, &netconfig, advert, sizeof(advert));
                     printf("create_network(): 0x%x\n", rc2);
                 }
                 if (kDown & KEY_B) {
                     rc2 = connect_network(&filter, &sec_config, &user_config, advert, sizeof(advert));
                     printf("connect_network(): 0x%x\n", rc2);
                 }

                 if (R_SUCCEEDED(rc2)) {
                     rc2 = ldnGetNetworkInfo(&netinfo);
                     printf("ldnGetNetworkInfo(): 0x%x\n", rc2);
                 }

                 if (R_SUCCEEDED(rc2)) {
                     rc2 = ldnGetIpv4Address(&addr, &mask);
                     printf("ldnGetIpv4Address(): 0x%x\n", rc2);
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

                         if (sockfd >= 0) {
                             memset(&serv_addr, 0, sizeof(serv_addr));
                             serv_addr.sin_family = AF_INET;
                             serv_addr.sin_addr.s_addr = htonl(addr.addr | ~mask.mask); // Setup a broadcast addr. If you want to use addrs for nodes on the network, you can use: htonl(netinfo.nodes[i].ip_addr.addr)
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
        }

        if (R_SUCCEEDED(rc) && (kDown & KEY_X)) {
            if (sockfd >= 0) {
                close(sockfd);
                sockfd = -1;
            }
            leave_network();
        }

        if (R_SUCCEEDED(rc) && sockfd>=0 && (kDown & (KEY_DLEFT|KEY_DRIGHT|KEY_DUP|KEY_DDOWN))) {
            char tmpstr[32];
            memset(tmpstr, 0, sizeof(tmpstr));

            if (kDown & KEY_DLEFT) strncpy(tmpstr, "Button DLEFT pressed.", sizeof(tmpstr)-1);
            else if (kDown & KEY_DRIGHT) strncpy(tmpstr, "Button DRIGHT pressed.", sizeof(tmpstr)-1);
            else if (kDown & KEY_DUP) strncpy(tmpstr, "Button DUP pressed.", sizeof(tmpstr)-1);
            else if (kDown & KEY_DDOWN) strncpy(tmpstr, "Button DDOWN pressed.", sizeof(tmpstr)-1);

            ssize_t ret = sendto(sockfd, tmpstr, strlen(tmpstr), 0, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in));
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

            if (ret>0) printf("Received data: %s\n", tmpstr);
        }

        if (R_SUCCEEDED(rc) && R_SUCCEEDED(eventWait(&state_event, 0))) {
            LdnNodeLatestUpdate nodes[8]={0};
            Result rc2 = ldnGetNetworkInfoLatestUpdate(&netinfo, nodes, 8);
            printf("NetworkInfo was updated.\nldnGetNetworkInfoLatestUpdate(): 0x%x\n", rc2); // Errors can be ignored if currently not on a network.
            if (R_SUCCEEDED(rc2)) {
                for (u32 i=0; i<8; i++) printf("%u: %d\n", i, nodes[i].val); // If you want, you can run handling for netinfo.nodes[i] when val is non-zero.
            }
        }

        consoleUpdate(NULL);
    }

    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }

    leave_network();
    eventClose(&state_event);
    ldnExit();
    socketExit();
    consoleExit(NULL);
    return 0;
}
