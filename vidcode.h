#ifndef INCLUDE_vidcode_h__
#define INCLUDE_vidcode_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pthread.h>

typedef struct vidcode {
    int width, height;
    float progress;
    AVFrame *frame;
    uint8_t *buffer;
    char *input_file_path;
    pthread_t input_thread;
} vidcode;

int vidcode_create(vidcode **code_ptr, int width, int height);

int vidcode_free(vidcode *code);

int vidcode_input(vidcode *code, char *file_path);

int vidcode_output(vidcode *code, char *file_path);

int vidcode_stop(vidcode *code);

int vidcode_is_done(vidcode *code);

float vidcode_progress(vidcode *code);

#endif
