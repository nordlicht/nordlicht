#include "nordlicht.h"

void die(char *message) {
    fprintf(stderr, "nordlicht: %s\n", message);
}

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

struct nordlicht {
    int width, height;
    int mutable;

    char *input_file_path;
    char *output_file_path;

    frame *code;
    /*
    int frames_read;
    int frames_written;
    AVFrame *frame;
    AVFrame *frame_wide;
    uint8_t *buffer;
    uint8_t *buffer_wide;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
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

frame* nordlicht_slice(nordlicht *n, int column) {
    return frame_create(1, n->height, 200);
}

//////////////////////////////////////////////////////////

nordlicht* nordlicht_create(int width, int height) {
    //av_log_set_level(AV_LOG_QUIET);
    av_register_all();

    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->mutable = 1;
    n->code = frame_create(width, height, 0);

    return n;
}

int nordlicht_free(nordlicht *n) {
    frame_free(n->code);
    free(n);
}

int nordlicht_set_input(nordlicht *n, char *file_path) {
    assert_mutable(n);
    n->input_file_path = file_path;
}

int nordlicht_set_output(nordlicht *n, char *file_path) {
    assert_mutable(n);
    n->output_file_path = file_path;
}

float nordlicht_step(nordlicht *n) {
    n->mutable = 0;

    int i;
    frame *s;
    for(i = 0; i < n->width; i++) {
        s = nordlicht_slice(n, i);
        frame_copy(s, n->code, i, 0);
    }

    frame_write(n->code, n->output_file_path);
    return 1;
}
