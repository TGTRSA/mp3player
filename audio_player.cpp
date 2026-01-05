
#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>
#include "cstddef"
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

std::unordered_map<std::string, AVCodecID> ext_map = {
    {"mp3", AV_CODEC_ID_MP3},
    {"aac", AV_CODEC_ID_AAC},
    {"wav", AV_CODEC_ID_WMAV1}
};

struct AudioBytes {
    std::vector<std::byte> data;  // stores many bytes
};

struct CodecWrapper {
    const AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
};


// returns the file extension
const char* getExt(const char* f) {
    
    const char* ext = strrchr(f, '.');
    return ext ? ext + 1 : nullptr; // Deeply confusing: apparently this is a weird if-return statement where strrchr returns a pointer before the "." and adding to it moves the pointer by an index so we can get the actual extension
}


void decode(CodecWrapper w, AVPacket *packet, AVFrame **frame) {
    int ret, data_size;
    AudioBytes byteinfo;

    ret = avcodec_send_packet(w.ctx, packet);
    if (ret < 0) {
        std::cerr << "Could not send packet\n";
        return;
    }

    AVFrame* f = *frame;
    while (true) {
        ret = avcodec_receive_frame(w.ctx, f);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            std::cerr << "Error during decoding\n";
            exit(1);
        }

        data_size = av_get_bytes_per_sample(w.ctx->sample_fmt);
        if (data_size < 0) {
            std::cerr << "Failed to calculate data size\n";
            exit(1);
        }

        for (int i = 0; i < f->nb_samples; i++) {
            for (int ch = 0; ch < w.ctx->ch_layout.nb_channels; ch++) {
                std::byte* sample_ptr = reinterpret_cast<std::byte*>(f->data[ch] + data_size*i);
                for (int b = 0; b < data_size; ++b) {
                    byteinfo.data.push_back(sample_ptr[b]);
                }
            }
        }
    }
}
void open_codec(CodecWrapper wrapper){
    if (avcodec_open2(wrapper.ctx, wrapper.codec, NULL)<0){
        exit(0);
    }
    AVFrame *frame  = av_frame_alloc();
    AVPacket *pkt   = av_packet_alloc();
    std::cout << "Preparing memory allocation\n";
    if (!frame ||  !pkt){
        std::cerr << "Could not allocate space for reading" << std::endl;
    }else{
        void decode();
    }
    

   av_frame_free(&frame);
   av_packet_free(&pkt); 
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
    // Gets audio codec and alloctes it to the CodecWrapper struct
    CodecWrapper wrapper = createCodec(filepath);
    if (!wrapper.codec || !wrapper.ctx) {
        std::cerr << "Failed to create codec\n";
        return;
    }

    std::cout << "Decoder ready for " << filepath << "\n";

    open_codec(wrapper);

    avcodec_free_context(&wrapper.ctx);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: player <audiofile>\n";
        return 1;
    }

    playAudio(argv[1]);
    return 0;
}

