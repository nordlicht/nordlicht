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

// Allocate a new mediabarcode of specific width and height.
int mediabarcode_create(mediabarcode **code_ptr, int width, int height);

// Free a mediabarcode and all its resources.
int mediabarcode_free(mediabarcode *code);

// Set input media file. The barcode generation will start immediately.
int mediabarcode_input(mediabarcode *code, char *file_path);

// Set ouput file. So far, only PPM files are supported.
int mediabarcode_output(mediabarcode *code, char *file_path);

// Stop barcode generation.
int mediabarcode_stop(mediabarcode *code);

// Returns 0 if the barcode is written completely, 1 otherwise.
int mediabarcode_is_done(mediabarcode *code);

// Returns a float between 0 and 1 that represents the writing progress.
float mediabarcode_progress(mediabarcode *code);

#endif
