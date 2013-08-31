#include "nordlicht.h"

#define MAX_FILTER_SIZE 256

void die(char *message) {
    fprintf(stderr, "nordlicht: %s\n", message);
}

////////////////////////////////////////////////////////////////////////////

typedef struct frame {
    AVFrame *frame;
    uint8_t *buffer;
} frame;

frame* frame_create(int width, int height, int fill_color) {
    frame *f;
    f = malloc(sizeof(frame));

    f->frame = avcodec_alloc_frame();
    f->frame->width = width;
    f->frame->height = height;

    f->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, width, height));
    avpicture_fill((AVPicture *)f->frame, f->buffer, PIX_FMT_RGB24, width, height);
    memset(f->frame->data[0], fill_color, f->frame->linesize[0]*height);
    return f;
}

void frame_free(frame *f) {
    av_free(f->buffer);
    avcodec_free_frame(&f->frame);
    free(f);
}

// Copy f1 to f2, offset the upper left corner. f2 is modified.
// TODO: optimize
void frame_copy(frame *f1, frame *f2, int offset_x, int offset_y) {
    int x, y;
    for (x = 0; x < f1->frame->width; x++) {
        for (y = 0; y < f1->frame->height; y++) {
            int f2_pos = (y+offset_y)*f2->frame->linesize[0]+(x+offset_x)*3;
            int f1_pos = y*f1->frame->linesize[0]+x*3;
            f2->frame->data[0][f2_pos] = f1->frame->data[0][f1_pos];
            f2->frame->data[0][f2_pos+1] = f1->frame->data[0][f1_pos+1];
            f2->frame->data[0][f2_pos+2] = f1->frame->data[0][f1_pos+2];
        }
    }
}

frame* frame_scale(frame *f, int width, int height) {
    frame *f2 = frame_create(width, height, 0);
    struct SwsContext *sws_context = sws_getContext(f->frame->width, f->frame->height, PIX_FMT_RGB24,
            f2->frame->width, f2->frame->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);
    sws_scale(sws_context, (uint8_t const * const *)f->frame->data,
            f->frame->linesize, 0, f->frame->height, f2->frame->data,
            f2->frame->linesize);
    sws_freeContext(sws_context);
    return f2;
}

void frame_write(frame *f, char *file_path) {
    AVCodec *encoder = avcodec_find_encoder_by_name("png");
    AVCodecContext *encoder_context;
    encoder_context = avcodec_alloc_context3(encoder);
    encoder_context->width = f->frame->width;
    encoder_context->height = f->frame->height;
    encoder_context->pix_fmt = PIX_FMT_RGB24;
    if (avcodec_open2(encoder_context, encoder, NULL) < 0) {
        die("Could not open output codec.");
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
    uint8_t buffer[200000]; // TODO: Why this size?
    packet.size = avcodec_encode_video(encoder_context, buffer, 200000, code->frame);
    packet.data = buffer;
#else
    int got_packet = 0;
    avcodec_encode_video2(encoder_context, &packet, f->frame, &got_packet);
    if (! got_packet) {
        die("Encoding error.");
    }
#endif

    FILE *file;
    file = fopen(file_path, "wb");
    if (! file) {
        die("Could not open output file.");
    }
    fwrite(packet.data, 1, packet.size, file);
    fclose(file);

    av_free_packet(&packet);

    avcodec_close(encoder_context);
    av_free(encoder_context);
}

///////////////////////////////////////////////////////////////////////

struct nordlicht {
    int width, height;
    int frames_written;
    int mutable;

    char *input_file_path;
    char *output_file_path;

    frame *code;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
    /*
    struct SwsContext *sws_ctx;

    struct SwsContext *sws_ctx2;
    AVCodecContext *encoder_context;
    */
};

void assert_mutable(nordlicht *n) {
    if (! n->mutable) {
        die("After the first step, input and output are immutable.");
        exit(1);
    }
}

frame* get_frame(nordlicht *n, int column) {
    AVFrame *avframe= NULL;
    avframe = avcodec_alloc_frame();

    double fps = 1.0/av_q2d(n->format_context->streams[n->video_stream]->codec->time_base);
    double total_frames = n->format_context->duration/AV_TIME_BASE*fps;
    double frames_step = total_frames/n->width;

    long target_frame = frames_step*column;

    av_seek_frame(n->format_context, n->video_stream, target_frame*(1/av_q2d(n->format_context->streams[n->video_stream]->time_base)*av_q2d(n->format_context->streams[n->video_stream]->codec->time_base)), 0);

    AVPacket packet;
    int64_t frameTime = -1;

    int frameFinished = 0;
    int try = 0;
    try++;
    if (try > 100) {
        printf("giving up\n");
        return 0;
    }
    while(av_read_frame(n->format_context, &packet) >= 0) {
        if (packet.stream_index == n->video_stream) {
            avcodec_decode_video2(n->decoder_context, avframe, &frameFinished, &packet);
            frameTime = packet.dts*av_q2d(n->format_context->streams[n->video_stream]->time_base)/av_q2d(n->format_context->streams[n->video_stream]->codec->time_base)/2;
            printf("%ld/%ld\n", frameTime, target_frame);
            break;
        }
        av_free_packet(&packet);
    }
    av_free_packet(&packet);

    avframe->width = n->decoder_context->width;
    avframe->height = n->decoder_context->height;
    frame *f = frame_create(n->decoder_context->width, n->decoder_context->height, 0);
    f->frame = avframe;
    return f;
}

frame* get_slice(nordlicht *n, int column) {
    frame *f = get_frame(n, column+1);
    frame *tmp;

    while (f->frame->width != 1 || f->frame->height != n->height) {
        int w = f->frame->width/(MAX_FILTER_SIZE/2);
        if (w < 1) {
            w = 1;
        }
        int h = f->frame->height/(MAX_FILTER_SIZE/2);
        if (h < n->height)
            h = n->height;

        tmp = frame_scale(f, w, h);
        f = tmp;
    }
    return f;
}

//////////////////////////////////////////////////////////

nordlicht* nordlicht_create(int width, int height) {
    //av_log_set_level(AV_LOG_QUIET);
    av_register_all();

    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->frames_written = 0;
    n->mutable = 1;
    n->code = frame_create(width, height, 0);

    n->format_context = NULL;
    // TODO: null things

    return n;
}

int nordlicht_free(nordlicht *n) {
    frame_free(n->code);
    free(n);
}

int nordlicht_set_input(nordlicht *n, char *file_path) {
    assert_mutable(n);
    n->input_file_path = file_path;

    if (avformat_open_input(&n->format_context, n->input_file_path, NULL, NULL) != 0)
        return 0;
    printf("i\n");
    if (avformat_find_stream_info(n->format_context, NULL) < 0)
        return 0;
    n->video_stream = av_find_best_stream(n->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (n->video_stream == -1)
        return 0;
    n->decoder_context = n->format_context->streams[n->video_stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(n->decoder_context->codec_id);
    if (codec == NULL) {
        die("Unsupported codec!");
        return 0;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(n->decoder_context, codec, &optionsDict)<0)
        return 0;
}

int nordlicht_set_output(nordlicht *n, char *file_path) {
    assert_mutable(n);
    n->output_file_path = file_path;
}

float nordlicht_step(nordlicht *n) {
    n->mutable = 0;

    int last_frame = n->frames_written + 100;
    if (last_frame > n->width) {
        last_frame = n->width;
    }

    int i;
    frame *s;
    for(i = n->frames_written; i < last_frame; i++) {
        s = get_slice(n, i);
        frame_copy(s, n->code, i, 0);
    }

    frame_write(n->code, n->output_file_path);
    n->frames_written = last_frame;
    return 1.0*n->frames_written/n->width;
}
