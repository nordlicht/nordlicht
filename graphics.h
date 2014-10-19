#ifndef INCLUDE_graphics_h__
#define INCLUDE_graphics_h__

typedef struct {
    int width, height;
    unsigned char *data;
} image;

typedef struct {
    int length;
    unsigned char *data;
} column;

column* column_scale(column *c, int length);
void column_free(column *c);

column* compress_to_column(image *i);
column* compress_to_row(image *i);
column* cut_middle_column(image *i);

#endif
