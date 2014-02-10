#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeImage.h"

#include "nordlicht.h"

struct nordlicht {
    int width, height;
    char *filename;
    char *data;
};

nordlicht* nordlicht_init(char *filename, int width, int height) {
    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->filename = filename;
    n->data = calloc(width*height*3, 1);
}

void nordlicht_free(nordlicht *n) {
    free(n->data);
    free(n);
}

char* get_column(nordlicht *n, int i) {
    char *column;
    column = malloc(n->height*3);
    memset(column, i, n->height*3);
    return column;
}

int nordlicht_generate(nordlicht *n) {
    int x, y;
    for (x=0; x<n->width; x++) {
        char *column = get_column(n, x); // TODO: Fill memory directly, no need to memcpy
        memcpy(n->data+n->height*3*x, column, n->height*3);
    }
    return 0;
}

int nordlicht_write(nordlicht *n, char *filename) {
    FIBITMAP *bitmap = FreeImage_ConvertFromRawBits(n->data, n->height, n->width, n->height*3, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, 1);
    bitmap = FreeImage_Rotate(bitmap, -90, 0);
    FreeImage_FlipHorizontal(bitmap);
    FreeImage_Save(FIF_PNG, bitmap, filename, 0);
}
