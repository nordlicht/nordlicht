#ifndef INCLUDE_image_h__
#define INCLUDE_image_h__

#include <libavcodec/avcodec.h>

typedef struct image image;

image* image_init(const int width, const int height);
int image_width(const image *i);
int image_height(const image *i);
void image_to_bgra(unsigned char *target, const int width, const int height, const image *i, const int offset_x, const int offset_y);
image* image_from_bgra(const unsigned char *target, const int width, const int height);
void image_copy_avframe(const image *i, AVFrame *frame);
void image_set(const image *i, const int x, const int y, const unsigned char r, const unsigned char g, const unsigned char b);
image* image_scale(const image *i, int width, int height);
image *image_flip(const image *i);
image* image_column(const image *i, double percent);
int image_write_png(const image *i, const char *file_path);
void image_free(image *i);

#endif
