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

// Represents the entire media file => All of its "byte" data
struct Stream {
    AVFormatContext     *av_fmt_ctx     = NULL;
    AVMediaType         media_type      = AVMEDIA_TYPE_AUDIO;
    int                 audio_stream;
};

struct AlsaHandle {
    snd_pcm_t           *pcm;
    const char *        device_name = "default";
    snd_pcm_uframes_t   buffer_size = 1024; // size of frames: usgined int => either 1 or 2 stereo or mono
    snd_pcm_stream_t    stream_type = SND_PCM_STREAM_PLAYBACK;
    int                 mode        = 0 ;
};

struct AudioMetadata {
    int channels;
    int sample_rate;
};

Stream get_audio_streams(const char * filename) {
    int rc;
    Stream s;
    AVDictionary *dict_opt = NULL;

    s.av_fmt_ctx = avformat_alloc_context();
    std::cout << "allocating context to avformat\n";
    if (!s.av_fmt_ctx) {
        fprintf(stderr, "Could not allocate AVFormatContext\n");
        return s;
    }

    rc = avformat_open_input(&s.av_fmt_ctx,filename ,NULL , NULL);
    std::cout << "Opening avformat\n";
    if (rc<0){
        fprintf(stderr, "Could not open file stream\n");
        avformat_free_context(s.av_fmt_ctx);
    }

    avformat_find_stream_info(s.av_fmt_ctx, &dict_opt);
    std::cout << "Collecting streams\n";
    s.audio_stream = av_find_best_stream(
        s.av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0
    );

    return s;
}

void play_audio(AlsaHandle alsa, AudioMetadata audiometada, const void * pcm_data){
    float buffer;
    int rc;
    int offset = 0;
    while(offset< audiometada.sample_rate){ //doing loop for number of frames => sample rate
        std::cout << "Doing stuff";
        snd_pcm_uframes_t frames_to_write = alsa.buffer_size; // dynamic value for chunking 
        if(offset+alsa.buffer_size > alsa.buffer_size) {
            frames_to_write = alsa.buffer_size - audiometada.sample_rate;
        }
        rc = snd_pcm_writei(alsa.pcm, pcm_data, frames_to_write);
        if (rc == -EPIPE) {
            // buffer underrun
            fprintf(stderr, "Underrun occurred\n");
            snd_pcm_prepare(alsa.pcm);
        } else if (rc < 0) {
            fprintf(stderr, "Error writing to PCM device: %s\n", snd_strerror(rc));
        } else {
            offset += rc;
        }
    }

}

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
    Stream s;
    int rc;
    std::cout << "Starting program "<<std::endl ;

    const char* path = "/home/tash/c++/sdl_mp3player/mp3/01 Ruin.mp3"; 

    s = get_audio_streams(path);

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
