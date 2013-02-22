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

int vidcode_create(vidcode **code_ptr, int width, int height) {
    vidcode *code;

    code = malloc(sizeof(vidcode));
    code->width = width;
    code->height = height;
    code->input_file_path = NULL;
    code->input_thread = 0;

    code->frame = avcodec_alloc_frame();

    code->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, width, height));
    avpicture_fill((AVPicture *)code->frame, code->buffer, PIX_FMT_RGB24, width, height);

    *code_ptr = code;
    return 0;
}

int vidcode_free(vidcode *code) {
    av_free(code->buffer);
    avcodec_free_frame(&code->frame);
    free(code);
}

int decode_frame(AVFrame *frame, AVFormatContext *formatContext, AVCodecContext *codecContext, int videoStream, long time) {
    av_seek_frame(formatContext, -1, time+10000000 , 0);

    int frameFinished = 0;
    int try = 0;
    AVPacket packet;
    while(!frameFinished) {
        try++;
        if (try > 100) {
            return 0;
        }
        while(av_read_frame(formatContext, &packet) >= 0) {
            // Is this a packet from the video stream?
            if (packet.stream_index == videoStream) {
                // Decode video frame
                avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);
                break;
            }
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    return 1;
}

void *threaded_input(void *arg) {
    vidcode *code = arg;

    av_log_set_level(AV_LOG_QUIET);

    int frameWidth = 16;
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext  *pCodecCtx = NULL;
    int             i, videoStream;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL; 
    AVFrame         *pFrameWide = NULL;
    int             numBytes, numBytesWide;
    uint8_t         *buffer = NULL;
    uint8_t         *bufferWide = NULL;

    AVDictionary    *optionsDict = NULL;
    struct SwsContext      *sws_ctx = NULL;
    struct SwsContext      *sws_ctx2 = NULL;

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if (avformat_open_input(&pFormatCtx, code->input_file_path, NULL, NULL) != 0)
        return 0; // Couldn't open file

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return 0; // Couldn't find stream information

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, code->input_file_path, 0);

    // Find the first video stream
    videoStream = -1;
    for (i=0; i<pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    if (videoStream == -1)
        return 0; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return 0; // Codec not found
    }
    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return 0; // Could not open codec

    // Allocate video frame
    pFrame = avcodec_alloc_frame();

    // Allocate an AVFrame structure
    pFrameWide = avcodec_alloc_frame();

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, code->height);
    buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    numBytesWide =avpicture_get_size(PIX_FMT_RGB24, frameWidth*code->width, code->height);
    bufferWide = (uint8_t *)av_malloc(numBytesWide*sizeof(uint8_t));

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                             frameWidth, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    sws_ctx2 = sws_getContext(frameWidth*code->width, code->height, PIX_FMT_RGB24,
                              code->width, code->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    // Assign appropriate parts of buffer to image planes in pFrameWide
    // Note that pFrameWide is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameWide, bufferWide, PIX_FMT_RGB24, frameWidth*code->width, code->height);

    int64_t seconds_per_frame = pFormatCtx->duration/code->width;

    uint8_t *orig = pFrameWide->data[0];

    for (i=0; i<code->width; i++) {
    /* int stepSize = code->width; */
    /* while (stepSize>1) { */
    /*     for (i = stepSize/2; i<code->width-stepSize/2+1; i+=stepSize) { */
        code->progress = (float)i/code->width;

            if (decode_frame(pFrame, pFormatCtx, pCodecCtx, videoStream, i*seconds_per_frame)) {
                pFrameWide->data[0] = orig+i*frameWidth*3;
                // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                        pFrameWide->data, pFrameWide->linesize);
            }
        /* stepSize /= 2; */

            if (i%100 == 0 || i == code->width-1) {
                pFrameWide->data[0] = orig;
                sws_scale(sws_ctx2, (uint8_t const * const *)pFrameWide->data, pFrameWide->linesize, 0, code->height,
                        code->frame->data, code->frame->linesize);
                /* redrawFunc(); */
            }
    }

    av_free(buffer);
    av_free(bufferWide);
    av_free(pFrame);
    av_free(pFrameWide);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    /* doneFunc(); */
}

int vidcode_input(vidcode *code, char *file_path) {
    if (code->input_file_path != NULL && strcmp(file_path, code->input_file_path) == 0)
        return 0;

    if (!vidcode_is_done(code)) {
        pthread_cancel(code->input_thread);
    }

    code->input_file_path = file_path;

    pthread_create(&code->input_thread, NULL, &threaded_input, code);
}

int vidcode_is_done(vidcode *code) {
    // TODO there are many problems with this
    return code->input_thread == 0 || pthread_kill(code->input_thread, 0) == ESRCH;
}

float vidcode_progress(vidcode *code) {

}

#endif
