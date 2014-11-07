#ifndef INCLUDE_graphics_h__
#define INCLUDE_graphics_h__

#include <libavcodec/avcodec.h>

typedef struct image image;

image* image_init(const int width, const int height);
int image_width(const image *i);
int image_height(const image *i);
void image_bgra(unsigned char *target, const int width, const int height, const image *i, const int offset_x, const int offset_y);
void image_copy_avframe(const image *i, AVFrame *frame);
void image_set(const image *i, const int x, const int y, const unsigned char r, const unsigned char g, const unsigned char b);
image* image_scale(const image *i, const int width, const int height);
image* image_compress_to_column(const image *i);
image* image_compress_to_row(const image *i);
image* image_column(const image *i, double percent);
void image_free(image *i);

#endif
