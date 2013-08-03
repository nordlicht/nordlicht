#include "nordlicht.h"

#define FRAME_WIDTH 20

void init_libav() {
    av_log_set_level(AV_LOG_QUIET);
    av_register_all();
}

// Seek to a specific time in the formatContext and decode that frame to `frame`.
int decode_frame(AVFrame *frame, AVFormatContext *formatContext, AVCodecContext *codecContext, int video_stream, long time) {
    av_seek_frame(formatContext, -1, time, 0);

    int frameFinished = 0;
    int try = 0;
    AVPacket packet;
    while(!frameFinished) {
        try++;
        if (try > 100) {
            fprintf(stderr, "nordlicht: Could not decode frame.\n");
            return 0;
        }
        while(av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == video_stream) {
                avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);
                break;
            }
            av_free_packet(&packet);
        }
        av_free_packet(&packet);
    }
    return 1;
}

// Use decode_frame to get one frame for every pixel of the nordlicht's width.
// Scale those to FRAME_WIDTH px width and put them in `code->frame_wide`. We
// have to do this intermediate step because libswscale has a limitation of how
// extreme it can downscale.
void *threaded_input(void *arg) {
    nordlicht *code = arg;
    int i;

    AVFormatContext *format_context = NULL;
    if (avformat_open_input(&format_context, code->input_file_path, NULL, NULL) != 0)
        return 0;

    if (avformat_find_stream_info(format_context, NULL) < 0)
        return 0;

    int video_stream = -1;
    for (i=0; i<format_context->nb_streams; i++) {
        if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    if (video_stream == -1)
        return 0;

    AVCodecContext *codec_context = NULL;
    codec_context = format_context->streams[video_stream]->codec;

    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(codec_context->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "nordlicht: Unsupported codec!\n");
        return 0;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(codec_context, codec, &optionsDict)<0)
        return 0;

    AVFrame *frame = NULL; 
    frame = avcodec_alloc_frame();

    uint8_t *buffer = NULL;
    buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB24, codec_context->width, code->height)*sizeof(uint8_t));

    struct SwsContext *sws_ctx = NULL;
    sws_ctx = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt,
            FRAME_WIDTH, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    int64_t seconds_per_frame = format_context->duration/code->width;

    for (i=0; i<code->width; i++) {
        if (decode_frame(frame, format_context, codec_context, video_stream, i*seconds_per_frame)) {
            uint8_t *addr = code->frame_wide->data[0]+i*FRAME_WIDTH*3;
            sws_scale(sws_ctx, (const uint8_t * const*)frame->data, frame->linesize, 0, codec_context->height,
                    &addr, code->frame_wide->linesize);
        }
        code->frames_read = i+1;
    }

    av_free(buffer);
    av_free(frame);

    avcodec_close(codec_context);
    avformat_close_input(&format_context);
}

nordlicht* nordlicht_create(int width, int height) {
    init_libav();

    nordlicht *code;
    code = malloc(sizeof(nordlicht));

    code->width = width;
    code->height = height;
    code->frames_read = 0;
    code->frames_written = 0;
    code->input_file_path = NULL;
    code->output_file_path = NULL;
    code->input_thread = 0;
    code->frame = avcodec_alloc_frame();
    code->frame_wide = avcodec_alloc_frame();

    code->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, code->width, code->height));
    avpicture_fill((AVPicture *)code->frame, code->buffer, PIX_FMT_RGB24, code->width, code->height);
    code->buffer_wide = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, FRAME_WIDTH*code->width, code->height));
    avpicture_fill((AVPicture *)code->frame_wide, code->buffer_wide, PIX_FMT_RGB24, FRAME_WIDTH*code->width, code->height);
    memset(code->frame_wide->data[0], 0, code->frame_wide->linesize[0]*code->height);

    return code;
}

int nordlicht_free(nordlicht *code) {
    av_free(code->buffer);
    av_free(code->buffer_wide);
    avcodec_free_frame(&code->frame);
    avcodec_free_frame(&code->frame_wide);
    free(code);
    return 0;
}

int nordlicht_input(nordlicht *code, char *file_path) {
    if (code->input_file_path != NULL && strcmp(file_path, code->input_file_path) == 0)
        return 0;
    code->input_file_path = file_path;

    // Fill `frame_wide` black.
    memset(code->frame_wide->data[0], 0, code->frame_wide->linesize[0]*code->height);
    // Start to read the frames to `frame_wide`.
    pthread_create(&code->input_thread, NULL, &threaded_input, code);

    // Sleep to avoid calling nordlicht_step before there
    sleep(1);
}

int nordlicht_output(nordlicht *code, char *file_path) {
    code->output_file_path = file_path;
    return 0;
}

// Scale the `frame_wide` to the desired width and write the result to `output_file_path`.
float nordlicht_step(nordlicht *code) {
    struct SwsContext *sws_ctx2 = NULL;
    sws_ctx2 = sws_getContext(FRAME_WIDTH*code->width, code->height, PIX_FMT_RGB24,
            code->width, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    sws_scale(sws_ctx2, (uint8_t const * const *)code->frame_wide->data,
            code->frame_wide->linesize, 0, code->height, code->frame->data,
            code->frame->linesize);

    AVCodec *codec = avcodec_find_encoder_by_name("png");
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    codecContext->width = code->width;
    codecContext->height = code->height;
    codecContext->pix_fmt = PIX_FMT_RGB24;
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        fprintf(stderr, "nordlicht: Could not open output codec.\n");
        return -1;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
    int gotPacket = 0;
    avcodec_encode_video2(codecContext, &packet, code->frame, &gotPacket);

    FILE *file;
    file = fopen(code->output_file_path, "wb");
    if (!file) {
        fprintf(stderr, "nordlicht: Could not open output file.\n");
        return -1;
    }
    fwrite(packet.data, 1, packet.size, file);
    fclose(file);

    av_free_packet(&packet);

    code->frames_written = code->frames_read;
    return (float)code->frames_written/code->width;
}

int nordlicht_done(nordlicht *code) {
    return code->frames_written == code->width;
}