#include "graphics.h"
#include <stdlib.h>
#include <string.h>

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

image* image_scale(const image *i, const int width, const int height) {
    image *i2 = image_init(width, height);

    // TODO: clever scaling
    const float x_factor = 1.0*image_width(i)/width;
    const float y_factor = 1.0*image_height(i)/height;

    int x, y;
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            int orig_x = x*x_factor;
            int orig_y = y*y_factor;
            image_set(i2, x, y, image_get_r(i, orig_x, orig_y), image_get_g(i, orig_x, orig_y), image_get_b(i, orig_x, orig_y));
        }
    }
    return i2;
}

image* image_compress_to_column(const image *i) {
    image *i2 = image_init(1, image_height(i));

    int x, y;
    const int step = image_width(i)/10;
    for (y = 0; y < image_height(i); y++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (x = 0; x < image_width(i); x += step) {
            rsum += image_get_r(i, x, y);
            gsum += image_get_g(i, x, y);
            bsum += image_get_b(i, x, y);
        }
        int num = image_width(i)/step+1;
        image_set(i2, 0, y, rsum/num, gsum/num, bsum/num);
    }

    return i2;
}

image* image_compress_to_row(const image *i) {
    image *i2 = image_init(1, image_width(i));

    int x, y;
    const int step = image_height(i)/10;
    for (x = 0; x < image_width(i); x++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (y = 0; y < image_height(i); y += step) {
            rsum += image_get_r(i, x, y);
            gsum += image_get_g(i, x, y);
            bsum += image_get_b(i, x, y);
        }
        int num = image_height(i)/step+1; // TODO: +1?
        image_set(i2, 0, x, rsum/num, gsum/num, bsum/num);
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
