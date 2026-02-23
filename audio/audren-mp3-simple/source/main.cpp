#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// ==

#include <switch.h>
#include <mpg123.h>
#include <malloc.h>

// This sample requires switch-mpg123 installed. This can be done via the pacman install 'pacman -S switch-mpg123'
// Note the link in the Makefile 'LIBS	:= -lnx -lmpg123'

// Sample comes from this website:
// https://www.soundjay.com/magic-sound-effect.html

// Simple struct to store our mp3 file
struct Mp3File
{
    mpg123_handle* handle;
    int channels;
    long rate;
    float length;

    AudioDriverWaveBuf WaveBuff;
    int audrenMpid;
};

// ==

// Cleanup a given mpg123 handle
 inline void CleanupMPGHandle(mpg123_handle* handle)
{
    mpg123_close(handle);
    mpg123_delete(handle);
}

 // Load a given mp3 file - This will load, decode and assign the file to audren
Mp3File* LoadMP3File(AudioDriver* audioDriver, const char* filename)
{
    mpg123_handle* mpgHandle = NULL;
    int channels = 0;
    int encoding = 0;
    long rate = 0;
    int  err = MPG123_OK;

    // Create a new mpg123 handle, Open the file, get the format of the file
    if ((mpgHandle = mpg123_new(NULL, &err)) == NULL ||
         mpg123_open(mpgHandle, filename) != MPG123_OK ||
         mpg123_getformat(mpgHandle, &rate, &channels, &encoding) != MPG123_OK)
    {
        printf("Trouble with mpg123: %s with file: %s\n", mpgHandle == NULL ? mpg123_plain_strerror(err) : mpg123_strerror(mpgHandle), filename);
        CleanupMPGHandle(mpgHandle);
        return nullptr;
    }

    // Ensure that this output format will not change (it could, when we allow it).
    mpg123_format_none(mpgHandle);
    mpg123_format(mpgHandle, rate, channels, encoding);

    // Get the length of the file
    int length = mpg123_length(mpgHandle);
    if (length == MPG123_ERR)
    {
        printf("Trouble with mpg123 getting length of file: %s \n", filename);
        CleanupMPGHandle(mpgHandle);
        return nullptr;
    }

    // Create a buffer. Must be 4096 byte aligned.
    int buffer_size = ((length * channels) + 0xFFF) & ~0xFFF;
    unsigned char* buffer = (unsigned char*)memalign(0x1000, buffer_size);
    armDCacheFlush(buffer, buffer_size);

    // Read and decode the mp3 into the buffer, here we are just reading the entire file into memory. You can do this in chunks too for streaming audio.
    size_t done = 0;
    err = mpg123_read(mpgHandle, buffer, buffer_size, &done);
    if (err != MPG123_OK)
    {
        printf("file not read into buffer correctly\n");
        CleanupMPGHandle(mpgHandle);
        free((void*)buffer);
        return nullptr;
    }

    // Setup the MP3File
    Mp3File* mp3File = new Mp3File();

    // Set the mpg123 handle - We need this to release later 
    mp3File->handle = mpgHandle;

    // Set the metadata - this is just for information, it's not required
    mp3File->channels = channels;
    mp3File->rate = rate;
    mp3File->length = length / rate;

    // Setup the wave buffer that audren will use when playing this sound. It stores our allocated buffer and size
    mp3File->WaveBuff = { 0 };
    mp3File->WaveBuff.data_raw = (const void*)buffer;
    mp3File->WaveBuff.size = buffer_size;
    mp3File->WaveBuff.start_sample_offset = 0;
    mp3File->WaveBuff.end_sample_offset = done / 2;

    // Assin the memory to audren - this is important to let audren know what memory it's able to use. Store the Map ID to allow us to free later
    mp3File->audrenMpid = audrvMemPoolAdd(audioDriver, (void*)buffer, buffer_size);
    audrvMemPoolAttach(audioDriver, mp3File->audrenMpid);

    return mp3File;
}

// Unloads the mp3 file - returning audren resources and mpg123 resources. mp3 will no longer be valid.
void UnloadMP3File(AudioDriver* audioDriver, Mp3File* mp3)
{
    if (audioDriver == nullptr) return;
    if (mp3 == nullptr) return;

    // Detach and remove the memory from audren
    audrvMemPoolDetach(audioDriver, mp3->audrenMpid);
    audrvMemPoolRemove(audioDriver, mp3->audrenMpid);

    // Free all our memory and handles
    free((void*)mp3->WaveBuff.data_raw);
    CleanupMPGHandle(mp3->handle);
    delete mp3;
}

// ==

void PlayMp3File(AudioDriver* audioDriver, Mp3File* mp3, int id, bool loop)
{
    // Should the music loop
    mp3->WaveBuff.is_looping = loop;                            

    audrvVoiceStop(audioDriver, id);                            // Stop the previous music playing on this id
    audrvVoiceAddWaveBuf(audioDriver, id, &(mp3->WaveBuff));    // Set the WaveBuf to this id
    audrvVoiceStart(audioDriver, id);                           // Play the music assigned to this id - which is now our mp3 file
}

// ==

int main(void)
{
    // Initialize the default console
    consoleInit(NULL);

    // Initialize and mount the rom file system 'romfs:/' - This will give you access to the files in the romfs folder
    romfsInit();

    // Initialize mpg123
    mpg123_init();

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    // Initialize Audren
    printf("Simple audren demonstration with mpg123 for loading mp3 files\n\n");

    static const AudioRendererConfig arConfig =
    {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 24,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

    AudioDriver audioDriver;
    Result res = audrenInitialize(&arConfig);
    bool initedDriver = false;
    bool initedAudren = R_SUCCEEDED(res);
    if (!initedAudren)
    {
        printf("audrenInitialize Failed: %08" PRIx32 "\n", res);
    }
    else
    {
        printf("Audren Initialized\n");
        res = audrvCreate(&audioDriver, &arConfig, 2);
        initedDriver = R_SUCCEEDED(res);
        if (!initedDriver)
        {
            printf("audrvCreate Failed: %08" PRIx32 "\n", res);
        }
        else
        {
            static const u8 sink_channels[] = { 0, 1 };
            audrvDeviceSinkAdd(&audioDriver, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

            res = audrvUpdate(&audioDriver);
            printf("audrvUpdate: %" PRIx32 "\n", res);

            res = audrenStartAudioRenderer();
            printf("audrenStartAudioRenderer: %" PRIx32 "\n", res);

            audrvVoiceInit(&audioDriver, 0, 1, PcmFormat_Int16, 48000);
            audrvVoiceSetDestinationMix(&audioDriver, 0, AUDREN_FINAL_MIX_ID);
            audrvVoiceSetMixFactor(&audioDriver, 0, 1.0f, 0, 0);
            audrvVoiceSetMixFactor(&audioDriver, 0, 1.0f, 0, 1);
            audrvVoiceStart(&audioDriver, 0);
        }
    }

    // Load a Mp3 File
    Mp3File* music = LoadMP3File(&audioDriver, "romfs:/test.mp3");
    if (music == nullptr)
    {
        printf("Failed to load mp3 file\n");
    }
    else
    {
        printf("\nLoaded MP3 File Successfully. Details:\n");
        printf("Channels: %d\nRate: %ld\nLength: %f\nbuffer_size: %ld\nleftSamples: %d\noffset: %d\n\n",
               music->channels, music->rate, music->length, music->WaveBuff.size, music->WaveBuff.end_sample_offset, music->WaveBuff.start_sample_offset);
    }

    printf("Press A to play the mp3 file.\n");

    // Main loop
    while (appletMainLoop())
    {
        padUpdate(&pad); // Update the gamepad
        u64 kDown = padGetButtonsDown(&pad); // Get a bitmask of the buttons pressed this frame
        if (kDown & HidNpadButton_Plus) break; // If we pressed the + button - return to HB menu

        if (initedDriver && music != nullptr)
        {
            if (kDown & HidNpadButton_A)
            {
                PlayMp3File(&audioDriver, music, 0, false);
            }

            // Update Audren
            res = audrvUpdate(&audioDriver);
            if (R_FAILED(res)) printf("audrvUpdate Failed: %" PRIx32 "\n", res);
            if (music->WaveBuff.state == AudioDriverWaveBufState_Playing) printf("Played Samples = %" PRIu32 "\n", audrvVoiceGetPlayedSampleCount(&audioDriver, 0));
        }

        // Update the default console - sending a new frame
        consoleUpdate(NULL);
    }

    // Unload the MP3 file, releasing resources
    UnloadMP3File(&audioDriver, music);

    // Audren Cleanup releasing resources
    if (initedDriver) audrvClose(&audioDriver);
    if (initedAudren) audrenExit();

    // Close mpg123
    mpg123_exit();

    // Close romfs mount
    romfsExit();

    // Close default console and release resources
    consoleExit(NULL);
    return 0;
}
