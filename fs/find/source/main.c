#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <switch/services/fs.h>

const char* PADDING = "                                ";
const size_t PADDING_LEN = 32;

void recursivePrint(FsFileSystem* fs, const char* path, u8 depth);

int main() {
    gfxInitDefault();
    consoleInit(NULL);

    fsInitialize();

    // Create a struct to hold the SD fs and mount it
    FsFileSystem sd;
    Result res = fsMountSdcard(&sd);

    if (!res) {
        puts("/");
        recursivePrint(&sd, "/", 1);
    } else {
        printf("Could not mount sdcard (%d)\n", res);
    }

    puts("Done. Press + to exit");

    // Main loop
    while (appletMainLoop()) {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // Your code goes here

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}

void recursivePrint(FsFileSystem* fs, const char* path, u8 depth) {
    FsDir dir;
    Result res;
    char* newpath;

    // Open directory with flags to read both files and other directories
    res = fsFsOpenDirectory(fs, path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir);
    if (!res) {
        // Get number of entries in dir.
        u64 nEntries = 0;
        fsDirGetEntryCount(&dir, &nEntries);

        // Allocate memory for directory entries
        FsDirectoryEntry* dirContents = calloc(nEntries, sizeof(FsDirectoryEntry));

        // Actual read of directory entries
        res = fsDirRead(&dir, 0, &nEntries, nEntries, dirContents);
        if (!res && nEntries > 0) {
            // For each entry...
            for (int i = 0; i < nEntries; i++) {
                // Print its name padded with spaces
                fputs(PADDING + PADDING_LEN - depth, stdout); // Quick hax to avoid for () print (haha lol)
                fputs(dirContents[i].name, stdout);

                if (dirContents[i].type == ENTRYTYPE_DIR) {
                    // If it is a directory, print a trailing slash, build the full path, and call recursively.
                    fputs("/\n", stdout);

                    newpath = strcpy(malloc(strlen(path) + strlen(dirContents[i].name) + 1 + 1), path);
                    strcat(newpath, dirContents[i].name);
                    strcat(newpath, "/");
                    recursivePrint(fs, newpath, depth + 1);
                    free(newpath);
                } else {
                    // If it is a file, just print a newline
                    putc('\n', stdout);
                }
            }
        } else {
            // Verbose print if for some reason we were able to open a directory, but not to read its contents.
            fputs(PADDING + PADDING_LEN - depth, stdout);
            printf("Error listing contents of %s\n", path);
        }

        // Close directory. Failure to do this will make your program crash eventually, as there is a maximum of dirs
        //  you can keep open.
        fsDirClose(&dir);

        // Free list of contents.
        free(dirContents);
    } else {
        // If we can't open the directory, print a question mark.
        fputs(PADDING + PADDING_LEN - depth, stdout);
        puts("?");
    }
}
