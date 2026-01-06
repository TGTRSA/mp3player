
#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>
#include "cstddef"
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <sys/syscall.h>
#include <unistd.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <alsa/asoundlib.h>
}


const char* filename;

// Example usage
std::unordered_map<std::string, std::string> ffplayFmtMap = {
    {"s16",  "s16le"},
    {"s16p", "s16le"},
    {"s32",  "s32le"},
    {"s32p", "s32le"},
    {"fltp", "pcm_f32le"},
    {"flt",  "f32le"},
    {"dblp", "f64le"},
    {"dbl",  "f64le"}
};

std::unordered_map<std::string, AVCodecID> ext_map = {
    {"mp3", AV_CODEC_ID_MP3},
    {"aac", AV_CODEC_ID_AAC},
    {"wav", AV_CODEC_ID_WMAV1}
};


struct audioInfo {
    unsigned int sample_rate;
    int          channels;
    AVSampleFormat sample_fmt; 
};

struct DecodedData {
    AVFrame *frame;
    AVPacket *packet;
};

struct AlsaParams {
    snd_pcm_t *pcm;
    _snd_pcm_hw_params *params;
};

struct AudioBytes {
    std::vector<std::byte> data;  // stores many bytes
};

struct CodecWrapper {
    const AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
};

typedef enum {
    FMT_S16_LE,
    FMT_S24_LE,
    FMT_S32_LE,
    FMT_FLOAT_LE,
} AudioFormat;

snd_pcm_format_t to_alsa_format(AudioFormat fmt)
{
    switch (fmt) {
        case FMT_S16_LE:   return SND_PCM_FORMAT_S16_LE;
        case FMT_S24_LE:   return SND_PCM_FORMAT_S24_LE;
        case FMT_S32_LE:   return SND_PCM_FORMAT_S32_LE;
        case FMT_FLOAT_LE:return SND_PCM_FORMAT_FLOAT_LE;
        default:           return SND_PCM_FORMAT_UNKNOWN;
    }
}

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    struct sample_fmt_entry {
        AVSampleFormat sample_fmt;
        const char *fmt_be;
        const char *fmt_le;
    };

    static const sample_fmt_entry sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };

    *fmt = nullptr;

    for (const auto& entry : sample_fmt_entries) {
        if (sample_fmt == entry.sample_fmt) {
            *fmt = AV_NE(entry.fmt_be, entry.fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

audioInfo getAudioParams() {
    audioInfo audioMetadata;
    //std::string str(filename);
    std::string command = "ffprobe -v error -select_streams a:0 "
                          "-show_entries stream=sample_rate,channels,sample_fmt "
                          "-of csv=p=0 \"" + std::string(filename) + "\"";

    FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Error while reading pipe (seems empty)\n";
            exit(1);
        }    
    char buffer[128];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    if (!output.empty() && output.back() == '\n'){
        output.pop_back();
    }
    std::cout << "RAW OUTPUT: "<< output << std::endl;
    std::stringstream STREAM(output);

   
    std::string sample_rate_str;
    std::string channels_str;
    std::string sample_fmt_str;

    std::getline(STREAM, sample_rate_str, ',');
    std::getline(STREAM, channels_str, ',');
    std::getline(STREAM, sample_fmt_str, ',');

    audioMetadata.sample_rate = std::stoul(sample_rate_str);
    audioMetadata.channels    = std::stoi(channels_str);
    audioMetadata.sample_fmt  = av_get_sample_fmt(sample_fmt_str.c_str());

    /**
     * getline() is kind of like .split() in python
     * */
    if (audioMetadata.sample_fmt == AV_SAMPLE_FMT_NONE) {
        std::cerr << "Unknown sample format: " << sample_fmt_str << "\n";
        exit(1);
    }
    // ---- PRINT RESULTS ----
    std::cout << "Parsed metadata:\n";
    std::cout << "  Sample Format: " << audioMetadata.sample_fmt << "\n";
    std::cout << "  Sample Rate:   " << audioMetadata.sample_rate << "\n";
    std::cout << "  Channels:      " << audioMetadata.channels << "\n";
    
    std::cout << "END\n"; 
    return audioMetadata;

}

void set_alsa_params(audioInfo metadata){
    AlsaParams params;
    // Open pcm device
    snd_pcm_open(&params.pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);

    // Alloc HW Params
    snd_pcm_hw_params_alloca(&params.params);

    // fill with default
    snd_pcm_hw_params_any(params.pcm, params.params);
    
    snd_pcm_format_t alsa_fmt = to_alsa_format(params.format);

    if (alsa_fmt == SND_PCM_FORMAT_UNKNOWN) {
        // handle error
        std::cerr << "UNKNOWN PCM FORMAT";
    }

    // set params
    snd_pcm_hw_params_set_access(params.pcm, params.params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(params.pcm, params.params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(params.pcm, params.params, metadata.channels);
    snd_pcm_hw_params_set_rate_near(params.pcm, params.params, &metadata.sample_rate,0 );

    // Aplly
    snd_pcm_hw_params(params.pcm, params.params);

    //Cleanup
    snd_pcm_close(params.pcm, params.params);

}

void send_to_sd(){
    std::cout << "Starting audio param extraction process"; 
    audioInfo Params = getAudioParams();
    void set_alsa_params() 
}

// returns the file extension
const char* getExt(const char* f) {
    
    const char* ext = strrchr(f, '.');
    return ext ? ext + 1 : nullptr; // Deeply confusing: apparently this is a weird if-return statement where strrchr returns a pointer before the "." and adding to it moves the pointer by an index so we can get the actual extension
}


void decode(CodecWrapper w, AVPacket *packet, AVFrame *frame) {
    std::cout << "Attempting decoding process\n";
    int ret, data_size;
    AudioBytes byteinfo;

    ret = avcodec_send_packet(w.ctx, packet);
    if (ret < 0) {
        std::cerr << "Could not send packet\n";
        return;
    }

    //AVFrame* f = *frame;
    while (true) {
        // getting frames
        ret = avcodec_receive_frame(w.ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            std::cerr << "Error during decoding\n";
            exit(1);
        }
        // getting samples
        data_size = av_get_bytes_per_sample(w.ctx->sample_fmt);
        if (data_size < 0) {
            std::cerr << "Failed to calculate data size\n";
            exit(1);
        }
        // Writing to data structure
        for (int i = 0; i < frame->nb_samples; i++) {
            for (int ch = 0; ch < w.ctx->ch_layout.nb_channels; ch++) {
                std::byte* sample_ptr = reinterpret_cast<std::byte*>(frame->data[ch] + data_size*i);
                for (int b = 0; b < data_size; ++b) {
                    byteinfo.data.push_back(sample_ptr[b]);
                    }
                }
            }
        }
        send_to_sd();
        av_frame_unref(frame);

}
DecodedData open_codec(CodecWrapper wrapper){
    std::cout << "Beginning decoding process\n";
    DecodedData data;
    if (avcodec_open2(wrapper.ctx, wrapper.codec, NULL)<0){
        exit(0);
    }
    data.frame    = av_frame_alloc();
    data.packet   = av_packet_alloc();
    std::cout << "Preparing memory allocation\n";
    if (!data.frame ||  !data.packet){
        std::cerr << "Could not allocate space for reading" << std::endl;
    }
    return data;
}

CodecWrapper createCodec(const char* path) {
    CodecWrapper wrapper;

    const char* ext = getExt(path);
    if (!ext) {
        std::cerr << "No file extension\n";
        return wrapper;
    }

    auto it = ext_map.find(ext);
    if (it == ext_map.end()) {
        std::cerr << "Unsupported format: " << ext << "\n";
        return wrapper;
    }

    AVCodecID id = it->second;

    wrapper.codec = avcodec_find_decoder(id);
    if (!wrapper.codec) {
        std::cerr << "Decoder not found\n";
        return wrapper;
    }

    wrapper.ctx = avcodec_alloc_context3(wrapper.codec);
    if (!wrapper.ctx) {
        std::cerr << "Could not allocate codec context\n";
        return wrapper;
    }

    return wrapper;
}

void playAudio(const char* filepath) {
    DecodedData data;
    //AVFrame
    // Gets audio codec and alloctes it to the CodecWrapper struct
    CodecWrapper wrapper = createCodec(filepath);
    if (!wrapper.codec || !wrapper.ctx) {
        std::cerr << "Failed to create codec\n";
        return;
    }

    std::cout << "Decoder ready for " << filepath << "\n";

    data = open_codec(wrapper);
        
    decode(wrapper, data.packet, data.frame);
    av_frame_free(&data.frame);
    av_packet_free(&data.packet); 
    avcodec_free_context(&wrapper.ctx);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: player <audiofile>\n";
        return 1;
    }
    filename = argv[1];
    playAudio(argv[1]);
    return 0;
}

