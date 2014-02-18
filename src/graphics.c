#include "graphics.h"
#include "common.h"

column* column_scale(column *c, int length) {
    column *c2;
    c2 = malloc(sizeof(column));
    c2->data = malloc(length*3);
    c2->length = length;

    float factor = 1.0*c->length/length;

    int i;
    for(i=0; i<length; i++) {
        int nn = factor*3*i;
        nn -= nn%3;
        memcpy(c2->data+3*i, c->data+nn, 3);
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
    for (y=0; y<i->height; y++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (x=0; x<i->width; x+=step) {
            bsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            rsum += i->data[y*i->width*3+3*x+2];
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
    int step = i->height/20;
    for (x=i->width-1; x>=0; x--) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (y=0; y<i->height; y+=step) {
            bsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            rsum += i->data[y*i->width*3+3*x+2];
        }
        c->data[3*x+0] = rsum/(i->height/step+1);
        c->data[3*x+1] = gsum/(i->height/step+1);
        c->data[3*x+2] = bsum/(i->height/step+1);
    }

    return c;
}

column* compress_to_diagonal(image *i) {
    column *c;
    c = malloc(sizeof(column));
    c->data = malloc(i->width*3);
    c->length = i->width;

    float slope = 1.0*i->height/i->width;

    int x;
    for (x=0; x<i->width; x++) {
        int y = x*slope;
        c->data[3*x+0] = i->data[y*i->width*3+3*x+2];
        c->data[3*x+1] = i->data[y*i->width*3+3*x+1];
        c->data[3*x+2] = i->data[y*i->width*3+3*x+0];
    }

    return c;
}

