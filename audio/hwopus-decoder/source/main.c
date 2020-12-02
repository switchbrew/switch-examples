#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <switch.h>
#include <opus/opusfile.h>

#include "sample_opus.h"

// Sample comes from this website:
// https://www.soundjay.com/magic-sound-effect.html

//Example for playing audio decoded with hwopus using audren. This decodes Opus in hardware. For encoding this is not available with hwopus, hence that has to be done in software.
//Requires package switch-opusfile.
//This uses libopusfile, see also the docs for that.
//Note that actual apps should handle audio on a dedicated thread.

static size_t opuspkt_tmpbuf_size = sizeof(HwopusHeader) + 4096*48;
static u8* opuspkt_tmpbuf;

int hw_decode(void *_ctx, OpusMSDecoder *_decoder, void *_pcm, const ogg_packet *_op, int _nsamples, int _nchannels, int _format, int _li) {
    HwopusDecoder *decoder = (HwopusDecoder*)_ctx;
    HwopusHeader *hdr = NULL;
    size_t pktsize, pktsize_extra;

    Result rc = 0;
    s32 DecodedDataSize = 0;
    s32 DecodedSampleCount = 0;

    if (_format != OP_DEC_FORMAT_SHORT) return OPUS_BAD_ARG;

    pktsize = _op->bytes;//Opus packet size.
    pktsize_extra = pktsize+8;//Packet size with HwopusHeader.

    if (pktsize_extra > opuspkt_tmpbuf_size) return OPUS_INTERNAL_ERROR;

    hdr = (HwopusHeader*)opuspkt_tmpbuf;
    memset(opuspkt_tmpbuf, 0, pktsize_extra);

    hdr->size = __builtin_bswap32(pktsize);
    memcpy(&opuspkt_tmpbuf[sizeof(HwopusHeader)], _op->packet, pktsize);

    rc = hwopusDecodeInterleaved(decoder, &DecodedDataSize, &DecodedSampleCount, opuspkt_tmpbuf, pktsize_extra, _pcm, _nsamples * _nchannels * sizeof(opus_int16));

    if (R_FAILED(rc)) return OPUS_INTERNAL_ERROR;
    if (DecodedDataSize != pktsize_extra || DecodedSampleCount != _nsamples) return OPUS_INVALID_PACKET;

    return 0;
}

int main(void)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    printf("Simple hwopus-decoder example with audren\n");

    static const AudioRendererConfig arConfig =
    {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 24,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

    size_t num_channels = 1;
    size_t samplerate = 48000;
    size_t max_samples = samplerate;
    size_t max_samples_datasize = max_samples*num_channels*sizeof(opus_int16);
    size_t mempool_size = (max_samples_datasize*2 + 0xFFF) &~ 0xFFF;//*2 for 2 wavebufs.
    void* mempool_ptr = memalign(0x1000, mempool_size);
    void* tmpdata_ptr = malloc(max_samples_datasize);
    opuspkt_tmpbuf = (u8*)malloc(opuspkt_tmpbuf_size);
    opus_int16* curbuf = NULL;

    AudioDriverWaveBuf wavebuf[2] = {0};
    int i, wavei;

    HwopusDecoder hwdecoder = {0};

    AudioDriver drv;
    Result res=0;
    bool initedDriver = false;
    bool initedAudren = false;
    bool audio_playing = false;

    int opret=0;
    int total_samples_size=0;
    OggOpusFile *of = NULL;

    if (mempool_ptr) memset(mempool_ptr, 0, mempool_size);
    if (tmpdata_ptr) memset(tmpdata_ptr, 0, max_samples_datasize);
    if (opuspkt_tmpbuf) memset(opuspkt_tmpbuf, 0, opuspkt_tmpbuf_size);

    if (mempool_ptr==NULL || tmpdata_ptr==NULL || opuspkt_tmpbuf==NULL) {
        res = 1;
        printf("Failed to allocate memory.\n");
    }

    if (R_SUCCEEDED(res)) {
        res = hwopusDecoderInitialize(&hwdecoder, samplerate, num_channels);//This assumes that the opus file is <samplerate> with <num_channels>.
        if (R_FAILED(res))
            printf("hwopusDecoderInitialize: %08" PRIx32 "\n", res);
        else
        {
            res = audrenInitialize(&arConfig);
            initedAudren = R_SUCCEEDED(res);
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

                    audrvVoiceInit(&drv, 0, num_channels, PcmFormat_Int16, samplerate);
                    audrvVoiceSetDestinationMix(&drv, 0, AUDREN_FINAL_MIX_ID);
                    if (num_channels == 1) {//mono
                        audrvVoiceSetMixFactor(&drv, 0, 1.0f, 0, 0);
                        audrvVoiceSetMixFactor(&drv, 0, 1.0f, 0, 1);
                    }
                    else {//stereo
                        audrvVoiceSetMixFactor(&drv, 0, 1.0f, 0, 0);
                        audrvVoiceSetMixFactor(&drv, 0, 0.0f, 0, 1);
                        audrvVoiceSetMixFactor(&drv, 0, 0.0f, 1, 0);
                        audrvVoiceSetMixFactor(&drv, 0, 1.0f, 1, 1);
                    }
                    audrvVoiceStart(&drv, 0);

                    for(i=0; i<2; i++) {
                        wavebuf[i].data_raw = mempool_ptr;
                        wavebuf[i].size = max_samples_datasize*2;//*2 for 2 wavebufs.
                        wavebuf[i].start_sample_offset = i * max_samples;
                        wavebuf[i].end_sample_offset = wavebuf[i].start_sample_offset + max_samples;
                    }
                }
            }
        }
    }

    if (initedDriver)
        printf("done. Press A to play a sound.\n");
    else
        printf("Init failed.\n");

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
                //Close the opus-file if needed and (re)open it, since libopusfile doesn't support seek-to-beginning.
                if (of)
                    op_free(of);
                of = op_open_memory (sample_opus, sample_opus_size, NULL);
                if (of == NULL) {
                    printf("Failed to open OggOpusFile.\n");
                }
                else {
                    op_set_decode_callback(of, hw_decode, &hwdecoder);

                    audrvVoiceStop(&drv, 0);
                    audio_playing = true;
                }
            }

            if (audio_playing) {
                wavei = -1;
                for(i=0; i<2; i++) {
                    if (wavebuf[i].state == AudioDriverWaveBufState_Free || wavebuf[i].state == AudioDriverWaveBufState_Done) {
                        wavei = i;
                        break;
                    }
                }

                if (wavei >= 0) {
                    curbuf = (opus_int16*)(mempool_ptr + wavebuf[wavei].start_sample_offset);

                    opret = op_read(of, tmpdata_ptr, max_samples * num_channels, NULL);//The buffer used here has to be seperate from mempool_ptr.
                    if (opret < 0)
                        printf("op_read() failed: %d\n", opret);
                    else if (opret == 0) {//End of file reached (see also libopusfile docs).
                        audio_playing = false;

                        if (of)
                            op_free(of);
                        of = NULL;
                    }
                    else {
                        if (opret > max_samples) opret = max_samples;//Should never happen.
                        total_samples_size = opret*sizeof(opus_int16);//Total samples data-size per channel.
                        memcpy(curbuf, tmpdata_ptr, total_samples_size);
                        armDCacheFlush(curbuf, total_samples_size);

                        wavebuf[wavei].end_sample_offset = wavebuf[wavei].start_sample_offset + total_samples_size/sizeof(opus_int16);

                        audrvVoiceAddWaveBuf(&drv, 0, &wavebuf[wavei]);
                    }
                }
                else {
                    if (!audrvVoiceIsPlaying(&drv, 0))
                        audrvVoiceStart(&drv, 0);
                }
            }

            res = audrvUpdate(&drv);
            if (R_FAILED(res))
                printf("audrvUpdate: %" PRIx32 "\n", res);
            if (audrvVoiceIsPlaying(&drv, 0))
                printf("sample count = %" PRIu32 "\n", audrvVoiceGetPlayedSampleCount(&drv, 0));
        }

        consoleUpdate(NULL);
    }

    hwopusDecoderExit(&hwdecoder);
    if (of)
        op_free(of);
    if (initedDriver)
        audrvClose(&drv);
    if (initedAudren)
        audrenExit();

    free(mempool_ptr);
    free(tmpdata_ptr);
    free(opuspkt_tmpbuf);

    consoleExit(NULL);
    return 0;
}
