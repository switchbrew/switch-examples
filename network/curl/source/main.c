// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
							
//include Curl library
#include <curl/curl.h>

//include for printfile
#include <errno.h>


//read "printfile" function by switchbrew for switch-examples (https://github.com/switchbrew/switch-examples/blob/master/fs/romfs).
void printfile(const char* path)
{
    FILE* f = fopen(path, "r");
    if (f)
    {
        char mystring[100];
        while (fgets(mystring, sizeof(mystring), f))
        {
            int a = strlen(mystring);
            if (mystring[a-1] == '\n')
            {
                mystring[a-1] = 0;
                if (mystring[a-2] == '\r')
                    mystring[a-2] = 0;
            }
            puts(mystring);
        }
        printf(">>EOF<<\n");
        fclose(f);
    } else {
        printf("errno is %d, %s\n", errno, strerror(errno));
    }
}


//curl "downloadFile" function by @segfault42 for NXDownload project (https://github.com/Dontwait00/nXDownload).
bool    downloadFile(const char *url, const char *filename)
{
    FILE                *dest = NULL;
    CURL                *curl = NULL;
    CURLcode            res = -1;

    consoleClear();

    curl = curl_easy_init();
    
    if (curl) {
        dest = fopen(filename, "wb");
        if (dest == NULL) {
            perror("fopen");
        } else {
            curl_easy_setopt(curl, CURLOPT_URL, url);                        // getting URL from char *url
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                    // useful for debugging
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);             // skipping cert. verification, if needed
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);             // skipping hostname verification, if needed
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, dest);                // writes pointer into FILE *destination
            
            res = curl_easy_perform(curl);                                    // perform tasks curl_easy_setopt asked before
            
            fclose(dest);
        }
    }

    curl_easy_cleanup(curl);                                                // Cleanup chunk data, resets functions.

    if (res != CURLE_OK) {
        printf("\n# Failed: %s%s%s\n", CONSOLE_RED, curl_easy_strerror(res), CONSOLE_RESET);
        remove(filename);
        return false;
    }
    
    return true;
}


int main()
{

    consoleInit(NULL);
	//Initialize curl
	socketInitializeDefault();

//print instructions.
printf("Curl Example\n");
printf("Press A to download [https://raw.githubusercontent.com/switchbrew/switch-examples/master/README.md].\nPress + to exit\n");


    while (appletMainLoop())
    {
        hidScanInput();

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

		if (kDown & KEY_A)
		{
			//download argument, downloadFile("URL", "sdmc:/path/to/file.txt");
            downloadFile("https://raw.githubusercontent.com/switchbrew/switch-examples/master/README.md", "sdmc:/switch/example.txt");
			consoleClear();
			printf("Showing the example downloaded!\n");
			printfile("sdmc:/switch/example.txt");
	}
			
			

        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
	//deinitialize curl
	socketExit();
    return 0;
}
