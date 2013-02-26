#ifndef INCLUDE_vidcode_h__
#define INCLUDE_vidcode_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pthread.h>

typedef struct vidcode {
    int width, height;
    int frames_read;
    int frames_written;
    AVFrame *frame;
    AVFrame *frame_wide;
    uint8_t *buffer;
    uint8_t *buffer_wide;
    char *input_file_path;
    char *output_file_path;
    pthread_t input_thread;
    pthread_t output_thread;
} vidcode;

int vidcode_create(vidcode **code_ptr, int width, int height);

int vidcode_free(vidcode *code);

int vidcode_input(vidcode *code, char *file_path);

int vidcode_output(vidcode *code, char *file_path);

int vidcode_stop(vidcode *code);

int vidcode_is_done(vidcode *code);

float vidcode_progress(vidcode *code);

#endif
