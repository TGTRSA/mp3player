#include <cstdint>
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


//file / stream
//   ↓ (libavformat)
//compressed packets (MP3, AAC, Opus…)
//   ↓ (libavcodec)
//decoded audio frames (AVFrame)
//   ↓ (libswresample, optional but common)
//raw PCM (S16_LE / FLOAT / etc.)
//
struct stream {
    AVFormatContext    *fmt_ctx    = NULL;
    AVMediaType         media_type = AVMEDIA_TYPE_AUDIO;
    int                 audio_stream;
};


struct FrameContainer {
    AVPacket *pckt;
    AVFrame *frame;
};

struct Decoder {
    AVCodecParameters *parameters;
    const AVCodec *codec;           // container for info (???)
    AVCodecContext *dec;            // codec information

};


struct AudioFormat {
    uint64_t        channel_layout;
    enum AVSampleFormat sample_fmt;
    int             sample_rate;
    int             channels;
};


int get_streams(const char* filename) {
    stream s;
    s.fmt_ctx;

    AVDictionary dict_opt=NULL;
    
    s.fmt_ctx = avformat_alloc_context();
    if (!s.fmt_ctx) {
        fprintf(stderr, "Could not allocate AVFormatContext\n");
        return -1;
    }
    rc = avformat_open_input(&s.fmt_ctx,filename ,NULL , NULL);
    if (rc<0){
        fprintf(stderr, "Could not open file stream\n");
        avformat_free_context(s.fmt_ctx);
    }

    avformat_find_stream_info(s.fmt_ctx, dict_opt);
    int audio_stream = av_find_best_stream(
        fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0
    );
    return audio_stream;
}

int main() {
    
    const char* filename = "/home/tash/c++/sdl_multiplayer/mp3/";

    int stream = get_streams(filename);
    printf("Stream: %d", stream);
    void convert_tp_pcm();

}
