#include <cstdint>
#include <iostream>
#include <alsa/asoundlib.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
#include <math.h>
#include <filesystem>

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

struct AlsaUtils {
    snd_pcm_t           *handler;
    const char*         device_name = "default";
    snd_pcm_uframes_t   buffer_size;
    snd_pcm_stream_t    stream_type ;
    int                 mode        = 0;
};

struct fileInformation {
    std::string codec;
    int sample_rate;
    int audio_channels;
};

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


void write_to_alsa(AlsaUtils alsa_params, fileInformation audio_data ) {
    int offset;
    alsa_params.buffer_size; 
    while (offset < audio_data.sample_rate){

    }
}

AlsaUtils set_params(fileInformation audio_data) {
    AlsaUtils alsa;
    alsa.stream_type = SND_PCM_STREAM_PLAYBACK;
    int rc = snd_pcm_open(&alsa.handler, alsa.device_name,alsa.stream_type , alsa.mode);
    if (rc < 0) {
        fprintf(stderr, "Could not open ALSA device: %s\n", snd_strerror(rc));
        
    }
    rc = snd_pcm_set_params(
        alsa.handler,
        SND_PCM_FORMAT_FLOAT_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        audio_data.audio_channels,
        audio_data.sample_rate,
        1,        // soft_resample
        50000     // latency in microseconds
    );
    if (rc < 0) {
        fprintf(stderr, "Unable to set PCM parameters: %s\n", snd_strerror(rc));
        
    };
    return alsa;

}

void play_music(AVFrame* pcm_data ,fileInformation audio_data){
   AlsaUtils alsa_params = set_params(audio_data); 
   write_to_alsa(alsa_params, audio_data);
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

int convert_to_pcm(Decoder d, Stream stream, fileInformation audio_metadata) {
    int ret, rc;
    AVPacket    *compressed_data_container; // container for compressed encoded data 
    AVFrame     *container_for_decompressed_data; //  container for pcm data

    AVSampleFormat sample_fmt = d.dec->sample_fmt;
    const char* sample_fmt_string = av_get_sample_fmt_name(sample_fmt);
    std::cout << "Sample format: " << sample_fmt_string << std::endl;

    avcodec_open2(d.dec, d.codec, nullptr); //initializes the decoder so they say

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
            //std::cout << container_for_decompressed_data;
            play_music(container_for_decompressed_data, audio_metadata);
            }
        av_packet_unref(compressed_data_container);
    }
    
    
    av_packet_free(&compressed_data_container);
    av_frame_free(&container_for_decompressed_data);
    return 0;
}

fileInformation get_file_info(const char* filename) {
    fileInformation audio_info;
    //av_register_all();
    
    AVFormatContext* format_ctx = nullptr;
    if(avformat_open_input(&format_ctx, filename, nullptr, nullptr) != 0) {
        std::cerr << "Could not open file" << std::endl;
    
    }
    
    if(avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream info" << std::endl;
        
    }
    
    // Find audio stream
    int audio_stream = -1;
    for(int i = 0; i < format_ctx->nb_streams; i++) {
        if(format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = i;
            break;
        }
    }
       if(audio_stream != -1) {
        AVCodecParameters* codec_params = format_ctx->streams[audio_stream]->codecpar;
     audio_info.codec = avcodec_get_name(codec_params->codec_id);
    audio_info.audio_channels = codec_params->ch_layout.nb_channels;
    audio_info.sample_rate = codec_params->sample_rate;

        std::cout << "Codec: " << audio_info.codec << std::endl;
        std::cout << "Channels: " << codec_params->ch_layout.nb_channels << std::endl;
        std::cout << "Sample Rate: " << codec_params->sample_rate << " Hz" << std::endl;
        std::cout << "Bitrate: " << codec_params->bit_rate << " bps" << std::endl;
        std::cout << "Duration: " << format_ctx->duration / AV_TIME_BASE << " seconds" << std::endl;
    }
    
    
    avformat_close_input(&format_ctx);
    return audio_info;
}

void create_pcm_handler() {
    AlsaUtils alsa;

    alsa.handler;
    alsa.device_name;
    //snd_pcm_t *pcm_handle;                  // ALSA device handle
    //const char* alsa_device_name = "default";
    //get_file_info();
    int sample_rate = 48000;
    int audio_channels = 2;
    snd_pcm_uframes_t frame_size = 1024;
    snd_pcm_stream_t stream_type = SND_PCM_STREAM_PLAYBACK;
    int mode = 0;
}


int main() {
    fileInformation metadata;
    const char* path = "/home/tash/c++/sdl_mp3player/mp3/01 Ruin.mp3";
    std::cout << path << std::endl;
    metadata=get_file_info(path);
}
