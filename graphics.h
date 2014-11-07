#ifndef INCLUDE_graphics_h__
#define INCLUDE_graphics_h__

typedef struct {
    int width, height;
    unsigned char *data;
} image;

image* image_init(const int width, const int height);
image* image_scale(const image *i, const int width, const int height);
image* image_compress_to_column(const image *i);
image* image_compress_to_row(const image *i);
image* image_column(const image *i, double percent);
void image_free(image *i);

#endif
