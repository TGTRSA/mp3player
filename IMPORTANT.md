# DECODING AUDIO INTO PCM

## Getting file streams
    ### What it requires:
        

    AVFormatContext *fmt = NULL;
    avformat_open_input(&fmt, filename, NULL, NULL);
    avformat_find_stream_info(fmt, NULL);

    int audio_stream = av_find_best_stream(
        fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0
    );

    Ideas:
        structs:
            streamContainter
                members: 
                    AVFormatContext = NULL
                    int             stream         
        function:
            get_streams()
            returns stream
        parameters:
            string ?? or const char*


## CREATE DECODER:
    ### What it requires:
        AVCodecParameters                                               
        AVCodec
        AVCodecContext
    ### What it requires:
        AVFormatContext
        int => audio_stream

    AVCodecParameters *par = fmt->streams[audio_stream]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(par->codec_id);

    AVCodecContext *dec = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(dec, par);
    avcodec_open2(dec, codec, NULL);

    Idaas:
        function:
            create_decoder() => decode():
                returns decoder:AVCodecContext while decode runs avcodec_open2
            decode()

## READ AND MAKE FRAME TYPE: => 
    ### What it requires:
        AVPacket
        AVFrame
    

    AVPacket *pkt = av_packet_alloc();
    AVFrame  *frame = av_frame_alloc();

    while (av_read_frame(fmt, pkt) >= 0) {
        if (pkt->stream_index == audio_stream) {
            avcodec_send_packet(dec, pkt);

            while (avcodec_receive_frame(dec, frame) == 0) {
                // frame now contains decoded audio samples
                convert_to_pcm()  <== this where frames convert
            }
        }
        av_packet_unref(pkt);
    }


# CONVERT FRAMES INTO PCM

SwrContext *swr = swr_alloc_set_opts(
    NULL,
    AV_CH_LAYOUT_STEREO,        // output layout
    AV_SAMPLE_FMT_S16,          // output format (PCM)
    44100,                      // output sample rate

    frame->channel_layout,
    frame->format,
    frame->sample_rate,

    0, NULL
);

swr_init(swr);
