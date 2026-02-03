//#include <cstdint>
#include <iostream>
#include <alsa/asoundlib.h>
#include <sched.h>
#include <useful_funcs.h>
//#include <typeinfo>
//#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
//#include <math.h>
#include <filesystem>

namespace  fs= std::filesystem;
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

    AVFormatContext     *av_fmt_ctx     = nullptr;

    AVMediaType         media_type      = AVMEDIA_TYPE_AUDIO;

    int                 audio_stream    = -1 ;
};

struct AlsaHandle {
    snd_pcm_t           *pcm;
    const char*         device_name = "default";
    snd_pcm_uframes_t   buffer_size = 1024; // size of frames: usgined int => either 1 or 2 stereo or mono
    snd_pcm_stream_t    stream_type = SND_PCM_STREAM_PLAYBACK;
    int                 mode        = 0 ;
};

struct AudioMetadata {
    int channels;
    int sample_rate;
};

Stream get_audio_streams(const char *filename) {
    Stream s;// Initialize with safe defaults
    int rc;

    // Open input: handles allocation if s.av_fmt_ctx is NULL
    rc = avformat_open_input(&s.av_fmt_ctx, filename, NULL, NULL);
    if (rc < 0) {
        char errbuf[256];
        av_strerror(rc, errbuf, sizeof(errbuf));
        std::cerr << "Could not open " << filename << ": " << errbuf << std::endl;
        return s; 
    }

    // Retrieve stream information
    if (avformat_find_stream_info(s.av_fmt_ctx, NULL) < 0) {
        std::cerr << "Could not find stream information" << std::endl;
        avformat_close_input(&s.av_fmt_ctx);
        return s;
    }

    // Find the best audio stream
    s.audio_stream = av_find_best_stream(s.av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (s.audio_stream < 0) {
        std::cerr << "Could not find an audio stream in " << filename << std::endl;
    }

    return s;
}

void play_audio(AlsaHandle alsa, AudioMetadata audiometada, const void * pcm_data){
    //float buffer;
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

int open_alsa_dev(AlsaHandle &alsa){
    // 1. Open PCM device

    int rc = snd_pcm_open(&alsa.pcm, alsa.device_name, alsa.stream_type, alsa.mode);
    if(rc<0){
        std::cout << "Error opening alsa";
        print_nl();
    }
    return rc;
}


// int argc, char[] * argv 
int main() {
    AlsaHandle alsa;
    AudioMetadata metadata;
    
    int rc;
    std::cout << "Starting program "<<std::endl ;

    const char* path = "/home/tash/c++/audio_player/mp3/01 Ruin.mp3"; 
    std::string string_path = path;
    fs::path existing_path(path);
    if(!(fs::exists(existing_path))){
        std::cout << "File does not exist" << std::endl;
        return 0;
    }else{
        std::cout << path << " exists" <<std::endl;
    }

    Stream s = get_audio_streams(path);
    if (s.audio_stream < 0){
        fprintf(stderr, "Error no audio streams");
        return 0;
    }

    AVCodecParameters *params = s.av_fmt_ctx->streams[s.audio_stream]->codecpar;
    metadata.channels = params->ch_layout.nb_channels; 
    metadata.sample_rate = params->sample_rate;


    rc = open_alsa_dev(alsa);
    std::cout << "Opening alsa dev" << std::endl;
    if (rc < 0) {
        fprintf(stderr, "Could not open ALSA device: %s\n", snd_strerror(rc));
    }
    std::cout << "Finished opening ALSA dev";
    print_nl();
    rc = configure_params(alsa,metadata);
    std::cout <<  "Configuring params for alsa" << std::endl;
    if (rc < 0) {
        fprintf(stderr, "Unable to set PCM parameters: %s\n", snd_strerror(rc));
        return 1;
    }


    return 0;
}
