#ifndef INCLUDE_mediabarcode_h__
#define INCLUDE_mediabarcode_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pthread.h>

typedef struct mediabarcode {
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
} mediabarcode;

int mbc_create(mediabarcode **code_ptr, int width, int height);

int mbc_free(mediabarcode *code);

int mbc_input(mediabarcode *code, char *file_path);

int mbc_output(mediabarcode *code, char *file_path);

int mbc_stop(mediabarcode *code);

int mbc_is_done(mediabarcode *code);

float mbc_progress(mediabarcode *code);

#endif
