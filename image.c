#include "image.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <libswscale/swscale.h>

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 8, 0)
void av_frame_get_buffer(AVFrame *frame, int magic) { avpicture_alloc((AVPicture *)frame, frame->format, frame->width, frame->height); }
void av_frame_copy(AVFrame *dst, AVFrame *src) { dst = src; }
#endif

#define MAX_FILTER_SIZE 256

struct image {
    AVFrame *frame;
};

image *image_init(const int width, const int height) {
    image *i;
    i = (image *) malloc(sizeof(image));

    i->frame = (AVFrame *) av_frame_alloc();
    i->frame->width = width;
    i->frame->height = height;
    i->frame->format = PIX_FMT_RGB24; // best choice?
    av_frame_get_buffer(i->frame, 16); // magic number?
    return i;
}

int image_width(const image *i) {
    return i->frame->width;
}

int image_height(const image *i) {
    return i->frame->height;
}

void image_set(const image *i, const int x, const int y, const unsigned char r, const unsigned char g, const unsigned char b) {
    *(i->frame->data[0]+y*i->frame->linesize[0]+x*3+0) = r;
    *(i->frame->data[0]+y*i->frame->linesize[0]+x*3+1) = g;
    *(i->frame->data[0]+y*i->frame->linesize[0]+x*3+2) = b;
}

unsigned char image_get_r(const image *i, const int x, const int y) {
    return *(i->frame->data[0]+y*i->frame->linesize[0]+x*3+0);
}

unsigned char image_get_g(const image *i, const int x, const int y) {
    return *(i->frame->data[0]+y*i->frame->linesize[0]+x*3+1);
}

unsigned char image_get_b(const image *i, const int x, const int y) {
    return *(i->frame->data[0]+y*i->frame->linesize[0]+x*3+2);
}

void image_to_bgra(unsigned char *target, const int width, const int height, const image *i, const int offset_x, const int offset_y) {
    int x, y;
    for (y = 0; y < image_height(i) && offset_y+y < height ; y++) {
        for (x = 0; x < image_width(i) && offset_x+x < width; x++) {
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+0) = image_get_b(i, x, y);
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+1) = image_get_g(i, x, y);
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+2) = image_get_r(i, x, y);
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+3) = 255;
        }
    }
}

image* image_from_bgra(const unsigned char *source, const int width, const int height) {
    image *i = image_init(width, height);
    int x, y;
    for (y = 0; y < image_height(i); y++) {
        for (x = 0; x < image_width(i); x++) {
            image_set(i, x, y, *(source+width*4*y+4*x+2), *(source+width*4*y+4*x+1), *(source+width*4*y+4*x+0));
        }
    }
    return i;
}

void image_copy_avframe(const image *i, AVFrame *frame) {
    av_frame_copy(i->frame, frame);
}

image* image_scale(const image *i, int width, int height) {
    int target_width = width;
    int target_height = height;

    image *i2 = NULL;
    image *tmp = (image *) i;
    do {
        width = target_width;
        height = target_height;
        if (image_width(tmp)/width > MAX_FILTER_SIZE) {
            width = image_width(tmp)/MAX_FILTER_SIZE+1;
        }
        if (image_height(tmp)/height > MAX_FILTER_SIZE) {
            height = image_height(tmp)/MAX_FILTER_SIZE+1;
        }

        i2 = image_init(width, height);

        struct SwsContext *sws_context = sws_getContext(image_width(tmp), image_height(tmp), tmp->frame->format,
                                                        image_width(i2), image_height(i2), i2->frame->format,
                                                        SWS_AREA, NULL, NULL, NULL);
        sws_scale(sws_context, (uint8_t const * const *)tmp->frame->data,
                tmp->frame->linesize, 0, tmp->frame->height, i2->frame->data,
                i2->frame->linesize);
        sws_freeContext(sws_context);

        if (tmp != i) {
            image_free(tmp);
        }

        tmp = i2;
    } while (image_width(i2) != target_width || image_height(i2) != target_height);

    return i2;
}

image *image_flip(const image *i) {
    image *i2 = image_init(image_height(i), image_width(i));
    int x, y;
    for (x = 0; x < image_width(i2); x++) {
        for (y = 0; y < image_height(i2); y++) {
            image_set(i2, x, y, image_get_r(i, y, x), image_get_g(i, y, x), image_get_b(i, y, x));
        }
    }
    return i2;
}

image* image_column(const image *i, double percent) {
    image *i2 = image_init(1, image_height(i));

    int y;
    const int x = image_width(i)*percent;
    for (y = 0; y < image_height(i); y++) {
        image_set(i2, 0, y, image_get_r(i, x, y), image_get_g(i, x, y), image_get_b(i, x, y));
    }

    return i2;
}

void image_write_png(const image *i, const char *file_path) {
    AVCodec *encoder = avcodec_find_encoder_by_name("png");
    AVCodecContext *encoder_context;
    encoder_context = avcodec_alloc_context3(encoder);
    encoder_context->width = i->frame->width;
    encoder_context->height = i->frame->height;
    encoder_context->pix_fmt = PIX_FMT_RGB24;
    if (avcodec_open2(encoder_context, encoder, NULL) < 0) {
        error("Could not open output codec.");
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
    uint8_t buffer[200000]; // TODO: Why this size?
    packet.size = avcodec_encode_video(encoder_context, buffer, 200000, i->frame);
    packet.data = buffer;
#else
    int got_packet = 0;
    avcodec_encode_video2(encoder_context, &packet, i->frame, &got_packet);
    if (! got_packet) {
        error("Encoding error.");
    }
#endif

    FILE *file;
    file = fopen(file_path, "wb");
    if (! file) {
        error("Could not open output file.");
    }
    fwrite(packet.data, 1, packet.size, file);
    fclose(file);

    av_free_packet(&packet);

    avcodec_close(encoder_context);
    av_free(encoder_context);
}

void image_free(image *i) {
    av_frame_free(&i->frame);
    free(i);
}
