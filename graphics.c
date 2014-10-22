#include "graphics.h"
#include <stdlib.h>
#include <string.h>

image* image_scale(const image *i, const int width, const int height) {
    image *i2;
    i2 = (image *) malloc(sizeof(image));
    i2->data = (unsigned char *) malloc(width*height*3);
    i2->width = width;
    i2->height = height;

    // TODO: clever scaling
    const float x_factor = 1.0*i->width/width; // heh.
    const float y_factor = 1.0*i->height/height; // heh.

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
    image *i2;
    i2 = (image *) malloc(sizeof(image));
    i2->data = (unsigned char *) malloc(i->height*3);
    i2->width = 1;
    i2->height = i->height;

    int x, y;
    const int step = i->width/20;
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
    image *i2;
    i2 = (image *) malloc(sizeof(image));
    i2->data = (unsigned char *) malloc(i->width*3);
    i2->width = 1;
    i2->height = i->width;

    int x, y;
    const int step = 1;
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

image* image_middle_column(const image *i) {
    image *i2;
    i2 = (image *) malloc(sizeof(image));
    i2->data = (unsigned char *) malloc(i->height*3);
    i2->width = 1;
    i2->height = i->height;

    int y;
    const int x = i->width/2;
    for (y = 0; y < i->height; y++) {
        i2->data[3*y+0] = i->data[y*i->width*3+3*x+0];
        i2->data[3*y+1] = i->data[y*i->width*3+3*x+1];
        i2->data[3*y+2] = i->data[y*i->width*3+3*x+2];
    }

    return i2;
}
