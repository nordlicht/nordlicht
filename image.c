#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <libswscale/swscale.h>

#define MAX_FILTER_SIZE 256

struct image {
    AVFrame *frame;
};

image *image_init(const int width, const int height) {
    image *i;
    i = (image *) malloc(sizeof(image));

    i->frame = av_frame_alloc();
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

void image_bgra(unsigned char *target, const int width, const int height, const image *i, const int offset_x, const int offset_y) {
    int x, y;
    for (y = 0; y < image_height(i) && offset_y+y < height ; y++) {
        for (x = 0; x < image_width(i) && offset_x+x < width; x++) {
            // BGRA pixel format:
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+0) = image_get_b(i, x, y);
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+1) = image_get_g(i, x, y);
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+2) = image_get_r(i, x, y);
            *(target+width*4*(offset_y+y)+4*(offset_x+x)+3) = 255;
        }
    }
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

void image_free(image *i) {
    av_frame_free(&i->frame);
    free(i);
}
