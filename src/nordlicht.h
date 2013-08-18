#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

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

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
    struct SwsContext *sws_ctx;

    struct SwsContext *sws_ctx2;
    AVCodecContext *encoder_context;
} nordlicht;

// Allocate a new nordlicht of specific width and height. Use
// `nordlicht_free` to free it again.
nordlicht* nordlicht_create(int width, int height);

// Free a nordlicht.
int nordlicht_free(nordlicht *code);

// Set input video file. All codecs supported by ffmpeg should work.
int nordlicht_set_input(nordlicht *code, char *file_path);

// Set ouput file. As for now, only PNG files are supported. Plan is to
// overload this function to support direct generation to in-memory data
// structures.
int nordlicht_set_output(nordlicht *code, char *file_path);

// Do one "step" of generation, producing a usable but possibly incomplete
// output. Returns a float between 0 and 1 representing how much of the
// nordlicht was generated. A value of 1 means the nordlicht is done.
float nordlicht_step(nordlicht *code);

#endif
