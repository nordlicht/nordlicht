#ifndef INCLUDE_frame_h__
#define INCLUDE_frame_h__

typedef struct frame {
    AVFrame *frame;
    uint8_t *buffer;
} frame;

#define MAX_FILTER_SIZE 256

frame* frame_create(int width, int height, int fill_color);

void frame_free(frame *f);

// Copy f1 to f2, offset the upper left corner. f2 is modified.
// TODO: optimize
void frame_copy(frame *f1, frame *f2, int offset_x, int offset_y);

frame* frame_scale_unsafe(frame *f, int width, int height);

frame* frame_scale(frame *f, int width, int height);

void frame_write(frame *f, char *file_path);

#endif
