#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <switch.h>

//This example shows how to access the built in storage.

int main(int argc, char **argv)
{
    consoleInit(NULL);

	//In this example we are mounting the user filesystem.
	//For more options see https://github.com/switchbrew/libnx/blob/master/nx/include/switch/services/fs.h#L244
	FsFileSystem nandFS;
	Result rc = fsOpenBisFileSystem(&nandFS, FsBisPartitionId_User, "");
	if(R_SUCCEEDED(rc))
	{
		//Mount the device as user. You can now access it the same way you access sdmc:/
		fsdevMountDevice("user", nandFS);
		
		//List all the files in the user filesystem.
		DIR* dir;
		struct dirent* ent;
		dir = opendir("user:/");
		if(dir==NULL)
		{
			printf("Failed to open dir.\n");
		}
		else
		{
			printf("Dir-listing for '':\n");
			while ((ent = readdir(dir)))
			{
				printf("d_name: %s\n", ent->d_name);
			}
			closedir(dir);
			printf("Done.\n");
		}
		
		//Don't forget to unmount the device once you're done with it.
		fsdevUnmountDevice("user");
	}
	else
	{
		printf("Failed to open filesystem");
	}
	
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
