#ifndef INCLUDE_mediastrip_h__
#define INCLUDE_mediastrip_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pthread.h>

typedef struct mediastrip {
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
} mediastrip;

/*
 * Allocate a new mediastrip of size width x height
 */
int mediastrip_create(mediastrip **code_ptr, int width, int height);

/*
 * Free a mediastrip and all its resources.
 */
int mediastrip_free(mediastrip *code);

/*
 * Set input media file. The strip generation will start immediately.
 */
int mediastrip_input(mediastrip *code, char *file_path);

/*
 * Set ouput file. So far, only PPM files are supported.
 */
int mediastrip_output(mediastrip *code, char *file_path);

/*
 * Stop input and output threads
 */
int mediastrip_stop(mediastrip *code);

/*
 * Returns 0 if the strip is written completely, 1 otherwise
 */
int mediastrip_is_done(mediastrip *code);

/*
 * Returns a float between 0 and 1 that represents the writing progress
 */
float mediastrip_progress(mediastrip *code);

#endif
