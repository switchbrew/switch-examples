#include <string.h>
#include <stdio.h>
#include <math.h>

#include <switch.h>

#define SAMPLERATE 48000
#define SAMPLESPERBUF (SAMPLERATE / 10)

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
    Handle event = 0;
    AudioOutBuffer source_buffer;
    AudioOutBuffer released_buffer;
    
    int notefreq[] = {
        220,
        440, 880, 1760, 3520, 7040,
        14080,
        7040, 3520, 1760, 880, 440
    };

    u32 raw_data[SAMPLESPERBUF * 2];
    fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[4]);

    gfxInitDefault();

    // Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
    consoleInit(NULL);

    rc = audoutInitialize();
    printf("audoutInitialize() returned 0x%x\n", rc);

    if (R_SUCCEEDED(rc))
    {
        rc = audoutRegisterBufferEvent(&event);
        printf("audoutRegisterBufferEvent() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        source_buffer.next = 0;
        source_buffer.buffer = raw_data;
        source_buffer.buffer_size = sizeof(raw_data);
        source_buffer.data_size = SAMPLESPERBUF * 2;
        source_buffer.data_offset = 0;
        
        rc = audoutAppendAudioOutBuffer(&source_buffer);
        printf("audoutAppendAudioOutBuffer() returned 0x%x\n", rc);
    }
    
    if (R_SUCCEEDED(rc))
    {
        rc = audoutStartAudioOut();
        printf("audoutStartAudioOut() returned 0x%x\n", rc);
    }
    
    bool play_tone = false;
    printf("Press A, B, Y, X, Left, Up, Right, Down, L, R, ZL or ZR to play a different tone.\n");
    
    while (appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS) break;
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_A)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[0]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_B)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[1]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_Y)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[2]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_X)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[3]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_DLEFT)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[4]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_DUP)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[5]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_DRIGHT)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[6]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_DDOWN)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[7]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_L)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[8]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_R)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[9]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_ZL)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[10]);
            play_tone = true;
        }
        
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_ZR)
        {
            fill_audio_buffer(raw_data, 0, SAMPLESPERBUF * 2, notefreq[11]);
            play_tone = true;
        }
        
        if (play_tone)
        {
            u64 time_now = svcGetSystemTick();
            while ((svcGetSystemTick() - time_now) < 1250000)
            {
                s32 index;
                Result do_wait = svcWaitSynchronization(&index, &event, 1, 10000000);
            
                if (R_SUCCEEDED(do_wait))
                {
                    svcResetSignal(event);
                    
                    u32 released_count = 0;
                    Result do_release = audoutGetReleasedAudioOutBuffer(&released_buffer, &released_count);
                    
                    while (R_SUCCEEDED(do_release) && (released_count > 0))
                    {
                        do_release = audoutGetReleasedAudioOutBuffer(&released_buffer, &released_count);
                        audoutAppendAudioOutBuffer(&source_buffer);
                    }
                }
            }
            
            play_tone = false;
        }
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }
    
    rc = audoutStopAudioOut();
    printf("audoutStopAudioOut() returned 0x%x\n", rc);

    audoutExit();

    gfxExit();
    return 0;
}

