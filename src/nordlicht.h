#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pthread.h>
//#include <libgen.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

struct nordlicht;

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
nordlicht* nordlicht_create(int width, int height);

// Free a nordlicht.
int nordlicht_free(nordlicht *code);

// Set input media file.
int nordlicht_input(nordlicht *code, char *file_path);

// Set ouput file. As for now, only PNG files are supported.
int nordlicht_output(nordlicht *code, char *file_path);

// Do one "step" of generation, producing a usable but incomplete output.
float nordlicht_step(nordlicht *code);

// Returns 1 if the nordlicht is complete, and 0 otherwise.
int nordlicht_done(nordlicht *code);

#endif
