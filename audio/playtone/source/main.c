#include <string.h>
#include <stdio.h>
#include <math.h>

#include <switch.h>

#define SAMPLERATE 48000
#define SAMPLESPERBUF (SAMPLERATE / 10)
#define BYTESPERSAMPLE 4

typedef struct {
    u16 left_ch;
    u16 right_ch;
} sample_t;

void fill_audio_buffer(void* audio_buffer, size_t offset, size_t size, int frequency) {
    u32* dest = (u32*) audio_buffer;

    for (int i = 0; i < size; i++) {
        // This is a simple sine wave, with a frequency of `frequency` Hz, and an amplitude 30% of maximum.
        s16 sample = 0.3 * 0x7FFF * sin(frequency * (2 * M_PI) * (offset + i) / SAMPLERATE);

        // Stereo samples are interleaved: left and right channels.
        dest[i] = (sample << 16) | (sample & 0xffff);
    }
}

int main(int argc, char **argv)
{
    Result rc = 0;
    AudioOutBuffer source_buffer;
    AudioOutBuffer released_buffer;
    
    int notefreq[] = {
        220,
        440, 880, 1760, 3520, 7040,
        14080,
        7040, 3520, 1760, 880, 440
    };
    
    // Make sure the sample buffer is aligned to 0x1000 bytes
    sample_t __attribute__((aligned(0x1000))) raw_data[((SAMPLESPERBUF * BYTESPERSAMPLE) + 0xfff) & ~0xfff];

    gfxInitDefault();

    // Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
    consoleInit(NULL);

    // Initialize the default audio output device
    rc = audoutInitialize();
    printf("audoutInitialize() returned 0x%x\n", rc);

    if (R_SUCCEEDED(rc))
    {
        printf("Sample rate: 0x%x\n", audoutGetSampleRate());
        printf("Channel count: 0x%x\n", audoutGetChannelCount());
        printf("PCM format: 0x%x\n", audoutGetPcmFormat());
        printf("Device state: 0x%x\n", audoutGetDeviceState());
        
        // Start audio playback.
        rc = audoutStartAudioOut();
        printf("audoutStartAudioOut() returned 0x%x\n", rc);
    }
    
    bool play_tone = false;
    printf("Press A, B, Y, X, Left, Up, Right, Down, L, R, ZL or ZR to play a different tone.\n");
    
    while (appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu
        
        if (kDown & KEY_A)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[0]);
            play_tone = true;
        }
        
        if (kDown & KEY_B)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[1]);
            play_tone = true;
        }
        
        if (kDown & KEY_Y)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[2]);
            play_tone = true;
        }
        
        if (kDown & KEY_X)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[3]);
            play_tone = true;
        }
        
        if (kDown & KEY_DLEFT)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[4]);
            play_tone = true;
        }
        
        if (kDown & KEY_DUP)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[5]);
            play_tone = true;
        }
        
        if (kDown & KEY_DRIGHT)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[6]);
            play_tone = true;
        }
        
        if (kDown & KEY_DDOWN)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[7]);
            play_tone = true;
        }
        
        if (kDown & KEY_L)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[8]);
            play_tone = true;
        }
        
        if (kDown & KEY_R)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[9]);
            play_tone = true;
        }
        
        if (kDown & KEY_ZL)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[10]);
            play_tone = true;
        }
        
        if (kDown & KEY_ZR)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[11]);
            play_tone = true;
        }
        
        if (play_tone)
        {
            // Prepare the audio data source buffer.
            source_buffer.next = 0;
            source_buffer.buffer = raw_data;
            source_buffer.buffer_size = sizeof(raw_data);
            source_buffer.data_size = SAMPLESPERBUF * 2;
            source_buffer.data_offset = 0;
        
            // Play this buffer once.
            rc = audoutPlayBuffer(&source_buffer, &released_buffer);
            play_tone = false;
            
            if (!R_SUCCEEDED(rc))
                printf("audoutPlayBuffer() returned 0x%x\n", rc);
        }
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }
    
    // Stop audio playback.
    rc = audoutStopAudioOut();
    printf("audoutStopAudioOut() returned 0x%x\n", rc);

    // Terminate the default audio output device.
    audoutExit();
    
    gfxExit();
    return 0;
}
