#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <switch.h>
#include "sample_bin.h"

// Sample comes from this website:
// https://www.soundjay.com/magic-sound-effect.html

int main(void)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("Simple audren demonstration program\n");

    static const AudioRendererConfig arConfig =
    {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 24,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

    size_t mempool_size = (sample_bin_size + 0xFFF) &~ 0xFFF;
    void* mempool_ptr = memalign(0x1000, mempool_size);
    memcpy(mempool_ptr, sample_bin, sample_bin_size);
    armDCacheFlush(mempool_ptr, mempool_size);

    AudioDriverWaveBuf wavebuf = {0};
    wavebuf.data_raw = mempool_ptr;
    wavebuf.size = sample_bin_size;
    wavebuf.start_sample_offset = 0;
    wavebuf.end_sample_offset = sample_bin_size/2;
    //wavebuf.is_looping = true;

    AudioDriver drv;
    Result res;
    res = audrenInitialize(&arConfig);
    bool initedDriver = false;
    bool initedAudren = R_SUCCEEDED(res);
    if (!initedAudren)
        printf("audrenInitialize: %08" PRIx32 "\n", res);
    else
    {
        printf("audren initted!\n");
        res = audrvCreate(&drv, &arConfig, 2);
        initedDriver = R_SUCCEEDED(res);
        if (R_FAILED(res))
            printf("audrvCreate: %08" PRIx32 "\n", res);
        else
        {
            int mpid = audrvMemPoolAdd(&drv, mempool_ptr, mempool_size);
            audrvMemPoolAttach(&drv, mpid);

            static const u8 sink_channels[] = { 0, 1 };
            /*int sink =*/ audrvDeviceSinkAdd(&drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

            res = audrvUpdate(&drv);
            printf("audrvUpdate: %" PRIx32 "\n", res);

            res = audrenStartAudioRenderer();
            printf("audrenStartAudioRenderer: %" PRIx32 "\n", res);

            audrvVoiceInit(&drv, 0, 1, PcmFormat_Int16, 48000);
            audrvVoiceSetDestinationMix(&drv, 0, AUDREN_FINAL_MIX_ID);
            audrvVoiceSetMixFactor(&drv, 0, 1.0f, 0, 0);
            audrvVoiceSetMixFactor(&drv, 0, 1.0f, 0, 1);
            audrvVoiceStart(&drv, 0);
        }
    }
    printf("done. Press A to play a sound.\n");

    // Main loop
    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break;

        if (initedDriver)
        {
            if (kDown & HidNpadButton_A)
            {
                audrvVoiceStop(&drv, 0);
                audrvVoiceAddWaveBuf(&drv, 0, &wavebuf);
                audrvVoiceStart(&drv, 0);
            }

            res = audrvUpdate(&drv);
            if (R_FAILED(res))
                printf("audrvUpdate: %" PRIx32 "\n", res);
            if (wavebuf.state == AudioDriverWaveBufState_Playing)
                printf("sample count = %" PRIu32 "\n", audrvVoiceGetPlayedSampleCount(&drv, 0));
        }

        consoleUpdate(NULL);
    }

    if (initedDriver)
        audrvClose(&drv);
    if (initedAudren)
        audrenExit();

    consoleExit(NULL);
    return 0;
}
