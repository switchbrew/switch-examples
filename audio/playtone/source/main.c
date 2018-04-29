#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include <switch.h>

#define SAMPLERATE 48000
#define CHANNELCOUNT 2
#define FRAMERATE (1000 / 30)
#define SAMPLECOUNT (SAMPLERATE / FRAMERATE)
#define BYTESPERSAMPLE 2

void fill_audio_buffer(void* audio_buffer, size_t offset, size_t size, int frequency) {
    if (audio_buffer == NULL) return;
    
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
    
    int notefreq[] = {
        220,
        440, 880, 1760, 3520, 7040,
        14080,
        7040, 3520, 1760, 880, 440
    };
    
    gfxInitDefault();

    // Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
    consoleInit(NULL);
    
    AudioOutBuffer audout_buf;
    AudioOutBuffer *audout_released_buf;
    
    // Make sure the sample buffer size is aligned to 0x1000 bytes.
    u32 data_size = (SAMPLECOUNT * CHANNELCOUNT * BYTESPERSAMPLE);
    u32 buffer_size = (data_size + 0xfff) & ~0xfff;
    
    // Allocate the buffer.
    u8* out_buf_data = memalign(0x1000, buffer_size);
    
    // Ensure buffers were properly allocated.
    if (out_buf_data == NULL)
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        printf("Failed to allocate sample data buffers\n");
    }
    
    if (R_SUCCEEDED(rc))
        memset(out_buf_data, 0, buffer_size);
    
    if (R_SUCCEEDED(rc))
    {
        // Initialize the default audio output device.
        rc = audoutInitialize();
        printf("audoutInitialize() returned 0x%x\n", rc);
    }
    
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
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu
        
        if (kDown & KEY_A)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[0]);
            play_tone = true;
        }
        
        if (kDown & KEY_B)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[1]);
            play_tone = true;
        }
        
        if (kDown & KEY_Y)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[2]);
            play_tone = true;
        }
        
        if (kDown & KEY_X)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[3]);
            play_tone = true;
        }
        
        if (kDown & KEY_DLEFT)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[4]);
            play_tone = true;
        }
        
        if (kDown & KEY_DUP)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[5]);
            play_tone = true;
        }
        
        if (kDown & KEY_DRIGHT)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[6]);
            play_tone = true;
        }
        
        if (kDown & KEY_DDOWN)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[7]);
            play_tone = true;
        }
        
        if (kDown & KEY_L)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[8]);
            play_tone = true;
        }
        
        if (kDown & KEY_R)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[9]);
            play_tone = true;
        }
        
        if (kDown & KEY_ZL)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[10]);
            play_tone = true;
        }
        
        if (kDown & KEY_ZR)
        {
            fill_audio_buffer(out_buf_data, 0, data_size, notefreq[11]);
            play_tone = true;
        }
        
        if (R_SUCCEEDED(rc) && play_tone)
        {
            // Prepare the audio data source buffer.
            audout_buf.next = NULL;
            audout_buf.buffer = out_buf_data;
            audout_buf.buffer_size = buffer_size;
            audout_buf.data_size = data_size;
            audout_buf.data_offset = 0;
            
            // Prepare pointer for the released buffer.
            audout_released_buf = NULL;
            
            // Play the buffer.
            rc = audoutPlayBuffer(&audout_buf, &audout_released_buf);
            
            if (R_FAILED(rc))
                printf("audoutPlayBuffer() returned 0x%x\n", rc);
            
            play_tone = false;
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
