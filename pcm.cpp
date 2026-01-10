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





//file / stream
//   ↓ (libavformat)
//compressed packets (MP3, AAC, Opus…)
//   ↓ (libavcodec)
//decoded audio frames (AVFrame)
//   ↓ (libswresample, optional but common)
//raw PCM (S16_LE / FLOAT / etc.)
//
struct Stream {
    AVFormatContext     *av_fmt_ctx    = NULL;
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
    std::cout << "Retrieving codec parameters";
    if (!decoder.parameters) {
        std::cerr << "Could not retrieve codec parameters\n";
    };
    print_success();

    // find codec
    decoder.codec    = avcodec_find_decoder(decoder.parameters->codec_id);
    if (!decoder.codec){
        std::cerr << "Could not evaluate codec" << std::endl;
    }
    // allocate decoder context
    decoder.dec   = avcodec_alloc_context3(decoder.codec);
    if (!decoder.dec){
        std::cerr << "Error retrieving decoder context" << std::endl;
    }

    // initialise decoder parameters from context
    if ((avcodec_parameters_to_context(decoder.dec, decoder.parameters)) < 0){
        std::cerr << "Error assigning parameters to context\n";
    }
    
    return decoder;

}

int convert_to_pcm(Decoder d, Stream stream) {
    int ret, rc;
    AVPacket    *compressed_data_container; // container for compressed encoded data 
    AVFrame     *container_for_decompressed_data; //  container for pcm data

    AVSampleFormat sample_fmt = d.dec->sample_fmt;
    const char* sample_fmt_string = av_get_sample_fmt_name(sample_fmt);
    std::cout << "Sample format: " << sample_fmt_string << std::endl;

    avcodec_open2(d.dec, d.codec, nullptr);

    if(!(compressed_data_container = av_packet_alloc()) ){
        std::cerr << "Could not allocate packets";
    }

    // gives the frame struct some capability to accept compressed packets??
    container_for_decompressed_data = av_frame_alloc();
    if (!container_for_decompressed_data){
        std::cerr << "Could not alloc frame";
    }else {
        std::cout << "Allocated frames successfully";
    }
    // i assume "gives" the frames to the encoded_data_container
    while ((av_read_frame(stream.av_fmt_ctx, compressed_data_container)) >= 0){

        // reads whats in the compressed data container and translates it into the decode state
        ret = avcodec_send_packet(d.dec, compressed_data_container);
        if (ret<0)
        {
            //[FIX]
            std::cerr << " Could not decode packets";
            break;
        }
        while((rc = avcodec_receive_frame(d.dec,container_for_decompressed_data)) >= 0){
            std::cout << container_for_decompressed_data;
            }
        av_packet_unref(compressed_data_container);
    }
                
    
    
    
    av_packet_free(&compressed_data_container);
    av_frame_free(&container_for_decompressed_data);
    return 0;
}


int main() {

    const char* path = "/home/tash/c++/sdl_mp3player/mp3/01 Ruin.mp3";
    std::cout << path << std::endl;
    //std::to_string(path);
    if (std::filesystem::exists(path)) {
        std::cout << path << " exists!\n";
        Stream stream = get_streams(path);
        std::cout<< "Stream: " << stream.audio_stream << std::endl;
        Decoder decoder = create_decoder(stream);
        convert_to_pcm(decoder, stream);
    } else {
        std::cout << path << " does NOT exist.\n";
    }
    

}
