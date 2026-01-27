//#include <cstdint>
#include <iostream>
#include <alsa/asoundlib.h>
#include <sched.h>
//#include <typeinfo>
//#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
//#include <math.h>
//#include <filesystem>

//#include <libavcodec/codec_par.h>
extern "C" {
#include <stdio.h>
#include <sndfile.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/packet.h>
    #include <libavcodec/codec_par.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/channel_layout.h>

//#include <libavutil/samplefmt.h>
}

struct AlsaHandle {
    snd_pcm_t           *pcm;
    const char *        device_name = "default";
    snd_pcm_uframes_t   buffer_size; // size of frames: usgined int => either 1 or 2 stereo or mono
    snd_pcm_stream_t    stream_type = SND_PCM_STREAM_PLAYBACK;
    int                 mode        = 0 ;
};

struct AudioMetadata {
    int channels;
    int sample_rate;
};

int configure_params(AlsaHandle alsa, AudioMetadata audiodata){
    int rc = snd_pcm_set_params(
        alsa.pcm,
        SND_PCM_FORMAT_FLOAT_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        audiodata.channels,
        audiodata.sample_rate,
        1,        // soft_resample
        50000     // latency in microseconds
    );
    return rc;
}

int open_alsa_dev(AlsaHandle alsa){
    // 1. Open PCM device
    int rc = snd_pcm_open(&alsa.pcm, alsa.device_name, alsa.stream_type, alsa.mode);
    return rc;
}


// int argc, char[] * argv 
int main() {
    AlsaHandle alsa;
    AudioMetadata metadata;
    int rc;
    std::cout << "Starting program "<<std::endl ;
    rc = open_alsa_dev(alsa);
    if (rc < 0) {
        fprintf(stderr, "Could not open ALSA device: %s\n", snd_strerror(rc));
    }

    rc = configure_params(alsa,metadata);
    if (rc < 0) {
        fprintf(stderr, "Unable to set PCM parameters: %s\n", snd_strerror(rc));
        return 1;
    }




    return 0;
}
