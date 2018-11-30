# sysmodule

This template is for a sysmodule where the output .nsp is used by Atmosphère at: `sdmc:/atmosphere/titles/<titleid>/exefs.nsp`. To load the sysmodule at boot, the following file should exist: `sdmc:/atmosphere/titles/<titleid>/flags/boot2`.

Where titleid is whatever you specify. Update the `$(TARGET).json` file with the titleid and any other changes.

