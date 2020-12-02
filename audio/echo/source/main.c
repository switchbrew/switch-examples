#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include <switch.h>

// Example for audio capture and playback.
// This example continuously records audio data from the default input device (see libnx audin.h),
// and sends it to the default audio output device (see libnx audout.h).

#define SAMPLERATE 48000
#define CHANNELCOUNT 2
#define FRAMERATE (1000 / 30)
#define SAMPLECOUNT (SAMPLERATE / FRAMERATE)
#define BYTESPERSAMPLE 2

int main(int argc, char **argv)
{
    Result rc = 0;

    // Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    AudioInBuffer audin_buf;
    AudioOutBuffer audout_buf;
    AudioInBuffer *released_in_buffer;
    AudioOutBuffer *released_out_buffer;
    u32 released_in_count;
    u32 released_out_count;

    // Make sure the sample buffer size is aligned to 0x1000 bytes.
    u32 data_size = (SAMPLECOUNT * CHANNELCOUNT * BYTESPERSAMPLE);
    u32 buffer_size = (data_size + 0xfff) & ~0xfff;

    // Allocate the buffers.
    u8* in_buf_data = memalign(0x1000, buffer_size);
    u8* out_buf_data = memalign(0x1000, buffer_size);

    // Ensure buffers were properly allocated.
    if ((in_buf_data == NULL) || (out_buf_data == NULL))
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        printf("Failed to allocate sample data buffers\n");
    }

    if (R_SUCCEEDED(rc))
    {
        memset(in_buf_data, 0, buffer_size);
        memset(out_buf_data, 0, buffer_size);
    }

    if (R_SUCCEEDED(rc))
    {
        // Initialize the default audio input device.
        rc = audinInitialize();
        printf("audinInitialize() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        // Initialize the default audio output device.
        rc = audoutInitialize();
        printf("audoutInitialize() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        // Start audio capture.
        rc = audinStartAudioIn();
        printf("audinStartAudioIn() returned 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc))
    {
        // Start audio playback.
        rc = audoutStartAudioOut();
        printf("audoutStartAudioOut() returned 0x%x\n", rc);
    }

    // Prepare the input buffer.
    audin_buf.next = NULL;
    audin_buf.buffer = in_buf_data;
    audin_buf.buffer_size = buffer_size;
    audin_buf.data_size = data_size;
    audin_buf.data_offset = 0;

    // Prepare the output buffer.
    audout_buf.next = NULL;
    audout_buf.buffer = out_buf_data;
    audout_buf.buffer_size = buffer_size;
    audout_buf.data_size = data_size;
    audout_buf.data_offset = 0;

    // Prepare pointers and counters for released buffers.
    released_in_buffer = NULL;
    released_out_buffer = NULL;
    released_in_count = 0;
    released_out_count = 0;

    // Append the initial input buffer.
    rc = audinAppendAudioInBuffer(&audin_buf);
    printf("audinAppendAudioInBuffer() returned 0x%x\n", rc);

    // Append the initial output buffer.
    rc = audoutAppendAudioOutBuffer(&audout_buf);
    printf("audoutAppendAudioOutBuffer() returned 0x%x\n", rc);

    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        // Wait for audio capture and playback to finish.
        audinWaitCaptureFinish(&released_in_buffer, &released_in_count, UINT64_MAX);
        audoutWaitPlayFinish(&released_out_buffer, &released_out_count, UINT64_MAX);

        // Copy the captured audio data into the playback buffer.
        if ((released_in_buffer != NULL) && (released_out_buffer != NULL))
            memcpy(released_out_buffer->buffer, released_in_buffer->buffer, released_in_buffer->data_size);

        // Append the released buffers again.
        audinAppendAudioInBuffer(released_in_buffer);
        audoutAppendAudioOutBuffer(released_out_buffer);

        consoleUpdate(NULL);
    }

    // Stop audio capture.
    rc = audinStopAudioIn();
    printf("audinStopAudioIn() returned 0x%x\n", rc);

    // Stop audio playback.
    rc = audoutStopAudioOut();
    printf("audoutStopAudioOut() returned 0x%x\n", rc);

    // Terminate the default audio devices.
    audinExit();
    audoutExit();

    consoleExit(NULL);
    return 0;
}
