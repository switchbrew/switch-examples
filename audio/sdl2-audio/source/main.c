// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// Include sdl2 headers
#include <SDL.h>
#include <SDL_mixer.h>

// Main program entrypoint
int main(int argc, char *argv[])
{
    // This example uses sdl2 library to play a mp3 file

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    Result rc = romfsInit();
    if (R_FAILED(rc))
        printf("romfsInit: %08X\n", rc);

    else
        printf("Press A button to play the sound!\n");

    // Start SDL with audio support
    SDL_Init(SDL_INIT_AUDIO);

    // Load support for the MP3 format
    Mix_Init(MIX_INIT_MP3);

    // open 44.1KHz, signed 16bit, system byte order,
    //  stereo audio, using 4096 byte chunks
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);

    // Load sound file to use
    // Sound from https://freesound.org/people/jens.enk/sounds/434610/
    Mix_Music *audio = Mix_LoadMUS("romfs:/test.mp3");

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

        if (kDown & HidNpadButton_A)
            Mix_PlayMusic(audio, 1); //Play the audio file

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Free the loaded sound
    Mix_FreeMusic(audio);

    // Shuts down SDL subsystems
    SDL_Quit();

    // Deinitialize and clean up resources used by the console (important!)
    romfsExit();
    consoleExit(NULL);
    return 0;
}
