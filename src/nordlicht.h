#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pthread.h>
//#include <libgen.h>
#include <string.h>

typedef struct nordlicht {
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
    int is_done;
} nordlicht;

// Allocate a new nordlicht of specific width and height.
int nordlicht_create(nordlicht **code_ptr, int width, int height);

// Free a nordlicht and all its resources.
int nordlicht_free(nordlicht *code);

// Set input media file. The barcode generation will start immediately.
int nordlicht_input(nordlicht *code, char *file_path);

// Set ouput file. As for now, only PNG files are supported.
int nordlicht_output(nordlicht *code, char *file_path);

// Stop barcode generation.
int nordlicht_stop(nordlicht *code);

// Returns a float between 0 and 1 that represents the writing progress.
float nordlicht_progress(nordlicht *code);

#endif
