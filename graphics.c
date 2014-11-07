#include "graphics.h"
#include <stdlib.h>
#include <string.h>

struct image {
    int width, height;
    unsigned char *data;
};

image *image_init(const int width, const int height) {
    image *i;
    i = (image *) malloc(sizeof(image));
    i->data = (unsigned char *) malloc(width*height*3);
    i->width = width;
    i->height = height;
    return i;
}

int image_width(const image *i) {
    return i->width;
}

int image_height(const image *i) {
    return i->height;
}

void image_bgra(unsigned char *target, const int width, const int height, const image *i, const int offset_x, const int offset_y) {
    int x, y;
    for (y = 0; y < image_height(i) && offset_y+y < height ; y++) {
        for (x = 0; x < image_width(i) && offset_x+x < width; x++) {
            // BGRA pixel format:
            memcpy(target+width*4*(offset_y+y)+4*(offset_x+x)+2, i->data+i->width*3*y+3*x+0, 1);
            memcpy(target+width*4*(offset_y+y)+4*(offset_x+x)+1, i->data+i->width*3*y+3*x+1, 1);
            memcpy(target+width*4*(offset_y+y)+4*(offset_x+x)+0, i->data+i->width*3*y+3*x+2, 1);
            memset(target+width*4*(offset_y+y)+4*(offset_x+x)+3, 255, 1);
        }
    }
}

void image_copy_avframe(const image *i, AVFrame *frame) {
    int y;
    for (y = 0; y < image_height(i); y++) {
        memcpy(i->data+y*i->width*3, frame->data[0]+y*frame->linesize[0], frame->linesize[0]);
    }
}

void image_set(const image *i, const int x, const int y, const unsigned char r, const unsigned char g, const unsigned char b) {
    i->data[y*i->width*3+x*3+0] = r;
    i->data[y*i->width*3+x*3+1] = g;
    i->data[y*i->width*3+x*3+2] = b;
}

image* image_scale(const image *i, const int width, const int height) {
    image *i2 = image_init(width, height);

    // TODO: clever scaling
    const float x_factor = 1.0*i->width/width;
    const float y_factor = 1.0*i->height/height;

    int x, y;
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            int orig_x = x*x_factor;
            int orig_y = y*y_factor;
            memcpy(i2->data + i2->width*y*3 + x*3, i->data + i->width*orig_y*3 + orig_x*3, 3);
        }
    }
    return i2;
}

image* image_compress_to_column(const image *i) {
    image *i2 = image_init(1, i->height);

    int x, y;
    const int step = i->width/10;
    for (y = 0; y < i->height; y++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (x = 0; x < i->width; x += step) {
            rsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            bsum += i->data[y*i->width*3+3*x+2];
        }
        i2->data[3*y+0] = rsum/(i->width/step+1);
        i2->data[3*y+1] = gsum/(i->width/step+1);
        i2->data[3*y+2] = bsum/(i->width/step+1);
    }

    return i2;
}

image* image_compress_to_row(const image *i) {
    image *i2 = image_init(i->width, 1);

    int x, y;
    const int step = i->height/10;
    for (x = 0; x < i->width; x++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (y = 0; y < i->height; y += step) {
            rsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            bsum += i->data[y*i->width*3+3*x+2];
        }
        i2->data[3*x+0] = rsum/(i->height/step);
        i2->data[3*x+1] = gsum/(i->height/step);
        i2->data[3*x+2] = bsum/(i->height/step);
    }

    return i2;
}

image* image_column(const image *i, double percent) {
    image *i2 = image_init(1, i->height);

    int y;
    const int x = i->width*percent;
    for (y = 0; y < i->height; y++) {
        i2->data[3*y+0] = i->data[y*i->width*3+3*x+0];
        i2->data[3*y+1] = i->data[y*i->width*3+3*x+1];
        i2->data[3*y+2] = i->data[y*i->width*3+3*x+2];
    }

    return i2;
}

void image_free(image *i) {
    free(i->data);
    free(i);
}
