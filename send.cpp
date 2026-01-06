#include <alsa/asoundlib.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
#include <math.h>
#include <math.h>
}


//bool negative = (int < 0);

struct AlsaInformation {
    snd_pcm_t           *handler;
    const char*         device_name = "default";
    snd_pcm_uframes_t   buffer_size;
    snd_pcm_stream_t    stream_type = SND_PCM_STREAM_PLAYBACK;
    int                 mode        = 0;
};

struct fileInformation {
    int sample_rate;
    int audio_channels;
};

int main() {
    printf("hello C\n");

    snd_pcm_t *pcm_handle;                  // ALSA device handle
    const char* alsa_device_name = "default";
    int sample_rate = 48000;
    int audio_channels = 2;
    snd_pcm_uframes_t frame_size = 1024;
    snd_pcm_stream_t stream_type = SND_PCM_STREAM_PLAYBACK;
    int mode = 0;

    // 1. Open PCM device
    int rc = snd_pcm_open(&pcm_handle, alsa_device_name, stream_type, mode);
    if (rc < 0) {
        fprintf(stderr, "Could not open ALSA device: %s\n", snd_strerror(rc));
        return 1;
    }

    // 2. Set hardware parameters
    rc = snd_pcm_set_params(
        pcm_handle,
        SND_PCM_FORMAT_FLOAT_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        audio_channels,
        sample_rate,
        1,        // soft_resample
        50000     // latency in microseconds
    );
    if (rc < 0) {
        fprintf(stderr, "Unable to set PCM parameters: %s\n", snd_strerror(rc));
        return 1;
    }

    // 3. Generate a 1-second 440Hz sine wave
    float buffer[48000 * 2]; // stereo
    for (int i = 0; i < 48000; i++) {
        float sample = 0.5f * sinf(2.0f * M_PI * 440.0f * i / 48000.0f);
        buffer[i * 2 + 0] = sample; // left
        buffer[i * 2 + 1] = sample; // right
    }

    // 4. Write to ALSA
    int offset = 0;
    while (offset < 48000) {
        snd_pcm_uframes_t frames_to_write = frame_size;
        if (offset + frame_size > 48000) frames_to_write = 48000 - offset;

        rc = snd_pcm_writei(pcm_handle, buffer + offset * audio_channels, frames_to_write);
        if (rc == -EPIPE) {
            // buffer underrun
            fprintf(stderr, "Underrun occurred\n");
            snd_pcm_prepare(pcm_handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error writing to PCM device: %s\n", snd_strerror(rc));
        } else {
            offset += rc;
        }
    }

    // 5. Cleanup
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    return 0;
}
