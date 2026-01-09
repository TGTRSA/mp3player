#include <cstdint>
#include <iostream>
#include <filesystem>
//#include <libavcodec/codec_par.h>
extern "C" {
#include <stdio.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/packet.h>

    #include <libavcodec/codec_par.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/channel_layout.h>

//#include <libavutil/samplefmt.h>
}


static AVFormatContext *av_fmt_ctx = NULL;
int audio_stream;


//file / stream
//   ↓ (libavformat)
//compressed packets (MP3, AAC, Opus…)
//   ↓ (libavcodec)
//decoded audio frames (AVFrame)
//   ↓ (libswresample, optional but common)
//raw PCM (S16_LE / FLOAT / etc.)
//
struct Stream {
    AVFormatContext    *av_fmt_ctx    = NULL;
    AVMediaType         media_type = AVMEDIA_TYPE_AUDIO;
    int                 audio_stream;
};


struct FrameContainer {
    AVPacket *pckt;
    AVFrame *frame;
};

struct Decoder {
    AVCodecParameters   *parameters;
    const AVCodec       *codec;           // container for info (???)
    AVCodecContext      *dec;            // codec information

};


struct AudioFormat {
    uint64_t        channel_layout;
    enum AVSampleFormat sample_fmt;
    int             sample_rate;
    int             channels;
};

void print_success(){
    std::cout << "SUCCESS\n";
}

Stream get_streams(const char* filename) {
    int rc;
    Stream stream;

    AVDictionary *dict_opt=NULL;

    /* open input file, and allocate format context */
    stream.av_fmt_ctx = avformat_alloc_context();
    std::cout << "allocating context to avformat\n";
    if (!stream.av_fmt_ctx) {
        fprintf(stderr, "Could not allocate AVFormatContext\n");
        return stream;
    }
    print_success();

    /* retrieve stream information */
    rc = avformat_open_input(&stream.av_fmt_ctx,filename ,NULL , NULL);
    std::cout << "Opening avformat\n";
    if (rc<0){
        fprintf(stderr, "Could not open file stream\n");
        avformat_free_context(stream.av_fmt_ctx);
    }
    print_success();

    avformat_find_stream_info(stream.av_fmt_ctx, &dict_opt);
    std::cout << "Collecting streams\n";
    stream.audio_stream = av_find_best_stream(
        stream.av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0
    );

    return stream;
}

Decoder create_decoder(Stream stream) {
    Decoder decoder;

    // get codec parameters
    decoder.parameters  = stream.av_fmt_ctx->streams[stream.audio_stream]->codecpar;

    // find codec
    decoder.codec    = avcodec_find_decoder(decoder.parameters->codec_id);

    // allocate decoder context
    decoder.dec     = avcodec_alloc_context3(decoder.codec);
    
    // initialise decoder parameters from context
    avcodec_parameters_to_context(decoder.dec, decoder.parameters);
    return decoder;

}


int main() {

    const char* path = "/home/tash/c++/sdl_mp3player/mp3/01 Ruin.mp3";
    std::cout << path << std::endl;
    //std::to_string(path);
    if (std::filesystem::exists(path)) {
        std::cout << path << " exists!\n";
        Stream stream = get_streams(path);
        std::cout<< "Stream: " << stream.audio_stream;
        void convert_tp_pcm();
    } else {
        std::cout << path << " does NOT exist.\n";
    }
    

}
