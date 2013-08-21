#include "nordlicht.h"

#define FRAME_WIDTH 10
#define STEP_SIZE 10

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
#define avcodec_free_frame av_freep
#endif

void init_libav() {
    av_log_set_level(AV_LOG_QUIET);
    av_register_all();
}

// Seek to a specific time in the formatContext and decode that frame to `frame`.
int decode_frame(AVFrame *frame, AVFormatContext *formatContext, AVCodecContext *codecContext, int video_stream, long time) {
    //av_seek_frame(formatContext, video_stream, time*(1/av_q2d(formatContext->streams[video_stream]->time_base)*av_q2d(formatContext->streams[video_stream]->codec->time_base)), AVSEEK_FLAG_BACKWARD);

    AVPacket packet;
    int64_t frameTime = -1;
    do {
        int frameFinished = 0;
        int try = 0;
            try++;
            if (try > 100) {
                return 0;
            }
            while(av_read_frame(formatContext, &packet) >= 0) {
                if (packet.stream_index == video_stream) {
                    avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);
                    frameTime = packet.dts*av_q2d(formatContext->streams[video_stream]->time_base)/av_q2d(formatContext->streams[video_stream]->codec->time_base)/2;
                    printf("%ld/%ld\n", frameTime, time);
                    break;
                }
                av_free_packet(&packet);
            }
            av_free_packet(&packet);
    } while(frameTime != time);
    return 1;
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
    code->frame = avcodec_alloc_frame();
    code->frame_wide = avcodec_alloc_frame();

    code->format_context = NULL;
    code->decoder_context = NULL;
    code->video_stream = -1;
    code->sws_ctx = NULL;

    code->sws_ctx2 = NULL;

    code->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, code->width, code->height));
    avpicture_fill((AVPicture *)code->frame, code->buffer, PIX_FMT_RGB24, code->width, code->height);
    code->buffer_wide = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, FRAME_WIDTH*code->width, code->height));
    avpicture_fill((AVPicture *)code->frame_wide, code->buffer_wide, PIX_FMT_RGB24, FRAME_WIDTH*code->width, code->height);
    memset(code->frame_wide->data[0], 0, code->frame_wide->linesize[0]*code->height);

    return code;
}

int nordlicht_free(nordlicht *code) {
    avcodec_close(code->decoder_context);
    avformat_close_input(&code->format_context);

    sws_freeContext(code->sws_ctx);
    sws_freeContext(code->sws_ctx2);
    av_free(code->buffer);
    av_free(code->buffer_wide);
    avcodec_free_frame(&code->frame);
    avcodec_free_frame(&code->frame_wide);
    avcodec_close(code->encoder_context);
    av_free(code->encoder_context);
    free(code);
    return 0;
}

int nordlicht_set_input(nordlicht *code, char *file_path) {
    if (code->input_file_path != NULL && strcmp(file_path, code->input_file_path) == 0)
        return 0;
    code->input_file_path = file_path;

    if (avformat_open_input(&code->format_context, code->input_file_path, NULL, NULL) != 0)
        return 0;

    if (avformat_find_stream_info(code->format_context, NULL) < 0)
        return 0;

    code->video_stream = av_find_best_stream(code->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (code->video_stream == -1)
        return 0;

    code->decoder_context = code->format_context->streams[code->video_stream]->codec;

    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(code->decoder_context->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "nordlicht: Unsupported codec!\n");
        return 0;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(code->decoder_context, codec, &optionsDict)<0)
        return 0;

    code->sws_ctx = sws_getContext(code->decoder_context->width, code->decoder_context->height, code->decoder_context->pix_fmt,
            FRAME_WIDTH, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);
}

int nordlicht_set_output(nordlicht *code, char *file_path) {
    code->output_file_path = file_path;

    code->sws_ctx2 = sws_getContext(FRAME_WIDTH*code->width, code->height, PIX_FMT_RGB24,
            code->width, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    AVCodec *encoder = avcodec_find_encoder_by_name("png");
    code->encoder_context = avcodec_alloc_context3(encoder);
    code->encoder_context->width = code->width;
    code->encoder_context->height = code->height;
    code->encoder_context->pix_fmt = PIX_FMT_RGB24;
    if (avcodec_open2(code->encoder_context, encoder, NULL) < 0) {
        fprintf(stderr, "nordlicht: Could not open output codec.\n");
        return -1;
    }

    return 0;
}

void write(nordlicht *code, AVPacket* packet) {
    FILE *file;
    file = fopen(code->output_file_path, "wb");
    if (!file) {
        fprintf(stderr, "nordlicht: Could not open output file.\n");
        exit(-1);
    }
    fwrite(packet->data, 1, packet->size, file);
    fclose(file);
}

float nordlicht_step(nordlicht *code) {
    // Use decode_frame to get one frame for every pixel of the nordlicht's width.
    // Scale those to FRAME_WIDTH px width and put them in `code->frame_wide`. We
    // have to do this intermediate step because libswscale has a limitation of how
    // extreme it can downscale.

    int i;

    //frameTime = packet.dts*av_q2d(formatContext->streams[video_stream]->time_base);
    double fps = 1.0/av_q2d(code->format_context->streams[code->video_stream]->codec->time_base);
    double total_frames = code->format_context->duration/AV_TIME_BASE*fps;
    double frames_step = total_frames/code->width;

    //printf("%f tb, %f fps, %f frames, %f fs", 1.0/av_q2d(code->format_context->streams[code->video_stream]->time_base), fps, total_frames, frames_step);

    AVFrame *frame = NULL;
    frame = avcodec_alloc_frame();

    int from_frame = code->frames_read + 1;
    int to_frame = from_frame + STEP_SIZE;
    if (to_frame > code->width)
        to_frame = code->width;

    for (i=from_frame; i<=to_frame; i++) {
        if (decode_frame(frame, code->format_context, code->decoder_context, code->video_stream, round(i*frames_step))) {
            uint8_t *addr = code->frame_wide->data[0]+i*FRAME_WIDTH*3;
            sws_scale(code->sws_ctx, (const uint8_t * const*)frame->data, frame->linesize, 0, code->decoder_context->height,
                    &addr, code->frame_wide->linesize);
        }
        code->frames_read = i;
    }

    av_free(frame);

    // Scale the `frame_wide` to the desired width and write the result to `output_file_path`.

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
    int gotPacket = 0;

    sws_scale(code->sws_ctx2, (uint8_t const * const *)code->frame_wide->data,
            code->frame_wide->linesize, 0, code->height, code->frame->data,
            code->frame->linesize);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
    uint8_t buffer[200000]; // TODO: Why this size?
    packet.size = avcodec_encode_video(codecContext, buffer, 200000, code->frame);
    packet.data = buffer;
#else
    avcodec_encode_video2(code->encoder_context, &packet, code->frame, &gotPacket);
#endif

    write(code, &packet);
    av_free_packet(&packet);

    code->frames_written = code->frames_read;
    return (float)code->frames_written/code->width;
}
