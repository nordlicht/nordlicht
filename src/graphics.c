#include "graphics.h"
#include "common.h"

column* column_scale(column *c, int length) {
    column *c2;
    c2 = malloc(sizeof(column));
    c2->data = malloc(length*3);
    c2->length = length;

    float factor = 1.0*c->length/length;

    int i;
    for (i = 0; i < length; i++) {
        int lower = factor*i + 0.5;
        int upper = factor*(i+1) - 0.5;

        if (lower > upper) {
            // this can happen when upscaling. pick nearest-neighbour entry:
            lower = upper = factor*(i+0.5);
        }

        int rsum = 0;
        int gsum = 0;
        int bsum = 0;
        int j;
        for (j = lower; j <= upper; j++) {
            rsum += c->data[j*3+0];
            gsum += c->data[j*3+1];
            bsum += c->data[j*3+2];
        }
        c2->data[i*3+0] = rsum/(upper-lower+1);
        c2->data[i*3+1] = gsum/(upper-lower+1);
        c2->data[i*3+2] = bsum/(upper-lower+1);
    }

    return c2;
}

void column_free(column *c) {
    free(c->data);
    free(c);
}

column* compress_to_column(image *i) {
    column *c;
    c = malloc(sizeof(column));
    c->data = malloc(i->height*3);
    c->length = i->height;

    int x, y;
    int step = i->width/20;
    for (y = 0; y < i->height; y++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (x = 0; x < i->width; x += step) {
            rsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            bsum += i->data[y*i->width*3+3*x+2];
        }
        c->data[3*y+0] = rsum/(i->width/step+1);
        c->data[3*y+1] = gsum/(i->width/step+1);
        c->data[3*y+2] = bsum/(i->width/step+1);
    }

    return c;
}

column* compress_to_row(image *i) {
    column *c;
    c = malloc(sizeof(column));
    c->data = malloc(i->width*3);
    c->length = i->width;

    int x, y;
    int step = 1;
    for (x = 0; x < i->width; x++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (y = 0; y < i->height; y += step) {
            rsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            bsum += i->data[y*i->width*3+3*x+2];
        }
        c->data[3*(i->width-x-1)+0] = rsum/(i->height/step);
        c->data[3*(i->width-x-1)+1] = gsum/(i->height/step);
        c->data[3*(i->width-x-1)+2] = bsum/(i->height/step);
    }

    return c;
}
