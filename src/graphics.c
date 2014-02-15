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
