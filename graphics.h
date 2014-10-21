#ifndef INCLUDE_graphics_h__
#define INCLUDE_graphics_h__

typedef struct {
    int width, height;
    unsigned char *data;
} image;

image* image_scale(image *i, int width, int height);
image* image_compress_to_column(image *i);
image* image_compress_to_row(image *i);
image* image_middle_column(image *i);

#endif
