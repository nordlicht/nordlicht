#include "vidcode.h"

void init_libav() {
    av_log_set_level(AV_LOG_QUIET);
    av_register_all();
}

int decode_frame(AVFrame *frame, AVFormatContext *formatContext, AVCodecContext *codecContext, int video_stream, long time) {
    av_seek_frame(formatContext, -1, time+10000000 , 0);

    int frameFinished = 0;
    int try = 0;
    AVPacket packet;
    while(!frameFinished) {
        try++;
        if (try > 100) {
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

void *threaded_input(void *arg) {
    vidcode *code = arg;
    int i;

    int frame_width = 16;

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
        fprintf(stderr, "Unsupported codec!\n");
        return 0;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(codec_context, codec, &optionsDict)<0)
        return 0;

    AVFrame *frame = NULL; 
    frame = avcodec_alloc_frame();

    AVFrame *frame_wide = NULL;
    frame_wide = avcodec_alloc_frame();

    uint8_t *buffer = NULL;
    buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB24, codec_context->width, code->height)*sizeof(uint8_t));

    uint8_t *buffer_wide = NULL;
    buffer_wide = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB24, frame_width*code->width, code->height)*sizeof(uint8_t));

    struct SwsContext *sws_ctx = NULL;
    sws_ctx = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt,
            frame_width, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    struct SwsContext *sws_ctx2 = NULL;
    sws_ctx2 = sws_getContext(frame_width*code->width, code->height, PIX_FMT_RGB24,
            code->width, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    avpicture_fill((AVPicture *)frame_wide, buffer_wide, PIX_FMT_RGB24, frame_width*code->width, code->height);

    int64_t seconds_per_frame = format_context->duration/code->width;
    uint8_t *orig = frame_wide->data[0];

    for (i=0; i<code->width; i++) {
        code->progress = (float)i/code->width;

        if (decode_frame(frame, format_context, codec_context, video_stream, i*seconds_per_frame)) {
            frame_wide->data[0] = orig+i*frame_width*3;
            sws_scale(sws_ctx, (uint8_t const * const *)frame->data, frame->linesize, 0, codec_context->height,
                    frame_wide->data, frame_wide->linesize);
        }

        if (i%100 == 0 || i == code->width-1) {
            frame_wide->data[0] = orig;
            sws_scale(sws_ctx2, (uint8_t const * const *)frame_wide->data, frame_wide->linesize, 0, code->height,
                    code->frame->data, code->frame->linesize);
        }
    }

    av_free(buffer);
    av_free(buffer_wide);
    av_free(frame);
    av_free(frame_wide);

    avcodec_close(codec_context);
    avformat_close_input(&format_context);
}

int vidcode_create(vidcode **code_ptr, int width, int height) {
    if (width <= 0 || height <= 0)
        return 1;

    init_libav();

    vidcode *code;
    code = malloc(sizeof(vidcode));

    code->width = width;
    code->height = height;
    code->progress = 0;
    code->input_file_path = NULL;
    code->input_thread = 0;
    code->frame = avcodec_alloc_frame();

    code->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, width, height));
    avpicture_fill((AVPicture *)code->frame, code->buffer, PIX_FMT_RGB24, width, height);

    *code_ptr = code;
    return 0;
}

int vidcode_free(vidcode *code) {
    av_free(code->buffer);
    avcodec_free_frame(&code->frame);
    free(code);
    return 0;
}

int vidcode_input(vidcode *code, char *file_path) {
    if (code->input_file_path != NULL && strcmp(file_path, code->input_file_path) == 0)
        return 0;

    if (!vidcode_is_done(code)) {
        pthread_cancel(code->input_thread);
    }

    code->input_file_path = file_path;
    pthread_create(&code->input_thread, NULL, &threaded_input, code);
}

int vidcode_is_done(vidcode *code) {
    // TODO there are many problems with this
    return code->input_thread == 0 || pthread_kill(code->input_thread, 0) == ESRCH;
}

float vidcode_progress(vidcode *code) {
    return code->progress;
}
