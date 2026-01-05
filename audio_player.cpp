
#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

std::unordered_map<std::string, AVCodecID> ext_map = {
    {"mp3", AV_CODEC_ID_MP3},
    {"aac", AV_CODEC_ID_AAC}
};

struct CodecWrapper {
    const AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
};

const char* getExt(const char* f) {
    const char* ext = strrchr(f, '.');
    return ext ? ext + 1 : nullptr; // Deeply confusing: apparently this is a weird if-return statement where strrchr returns a pointer before the "." and adding to it moves the pointer by an index so we can get the actual extension
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
    CodecWrapper wrapper = createCodec(filepath);
    if (!wrapper.codec || !wrapper.ctx) {
        std::cerr << "Failed to create codec\n";
        return;
    }

    std::cout << "Decoder ready for " << filepath << "\n";

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

