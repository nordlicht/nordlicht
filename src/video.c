#include "common.h"
#include "video.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
#define avcodec_free_frame av_freep
#endif

struct video {
    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
    AVFrame *frame;
};

video* video_init(char *filename) {
    av_log_set_level(AV_LOG_ERROR);
    av_register_all();

    video *f;
    f = malloc(sizeof(video));
    f->format_context = NULL;

    if (avformat_open_input(&f->format_context, filename, NULL, NULL) != 0)
        return NULL;
    if (avformat_find_stream_info(f->format_context, NULL) < 0)
        return NULL;
    f->video_stream = av_find_best_stream(f->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (f->video_stream == -1)
        return NULL;
    f->decoder_context = f->format_context->streams[f->video_stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(f->decoder_context->codec_id);
    if (codec == NULL) {
        error("Unsupported codec!");
        return NULL;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(f->decoder_context, codec, &optionsDict) < 0)
        return NULL;

    f->frame = avcodec_alloc_frame();

    return f;
}

double fps(video *f) {
    return av_q2d(f->format_context->streams[f->video_stream]->r_frame_rate);
}

int total_number_of_frames(video *f) {
    double duration_sec = 1.0*f->format_context->duration/AV_TIME_BASE;
    return fps(f)*duration_sec;
}

double grab_next_frame(video *f) {
    int valid = 0;
    int got_frame = 0;

    AVPacket packet;
    av_init_packet(&packet);

    double pts;

    while (!valid) {
        if (av_read_frame(f->format_context, &packet) >= 0) {
            if(packet.stream_index == f->video_stream) {
                avcodec_decode_video2(f->decoder_context, f->frame, &got_frame, &packet);
                if (got_frame) {
                    pts = packet.pts;
                    // to sec:
                    pts = pts*(double)av_q2d(f->format_context->streams[f->video_stream]->time_base);
                    // to frame number:
                    pts = pts*fps(f);
                    valid = 1;
                }
                av_free_packet(&packet);
            } else {
                av_free_packet(&packet);
            }
        } else {
            return -1;
        }
    }

    return pts;
}

void seek(video *f, long min_frame_nr, long max_frame_nr) {
    double time_base = av_q2d(f->format_context->streams[f->video_stream]->time_base);

    avformat_seek_file(f->format_context, f->video_stream, min_frame_nr/fps(f)/time_base, (min_frame_nr+max_frame_nr)/2/fps(f)/time_base, max_frame_nr/fps(f)/time_base, AVSEEK_FLAG_BACKWARD);

    grab_next_frame(f);
}

image* get_frame(video *f, double min_percent, double max_percent) {
    seek(f, min_percent*total_number_of_frames(f), max_percent*total_number_of_frames(f));

    image *i;
    i = malloc(sizeof(image));

    i->width = f->frame->width;
    i->height = f->frame->height;
    i->data = malloc(i->height*i->width*3);

    AVFrame *frame;
    frame = avcodec_alloc_frame();
    frame->width = i->width;
    frame->height = i->height;
    frame->format = PIX_FMT_RGB24;

    uint8_t *buffer;
    buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, i->width, i->height));
    avpicture_fill((AVPicture *)frame, buffer, PIX_FMT_RGB24, i->width, i->height);

    struct SwsContext *sws_context = sws_getContext(f->frame->width, f->frame->height, f->frame->format,
            f->frame->width, f->frame->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);
    sws_scale(sws_context, (uint8_t const * const *)f->frame->data,
            f->frame->linesize, 0, f->frame->height, frame->data,
            frame->linesize);
    sws_freeContext(sws_context);

    int y;
    for (y=0; y<i->height; y++) {
        memcpy(i->data+y*i->width*3, frame->data[0]+y*frame->linesize[0], frame->linesize[0]);
    }

    av_free(buffer);
    avcodec_free_frame(&frame);

    return i;
}

column* compress_to_column(image *i) {
    column *c;
    c = malloc(sizeof(column));
    c->data = malloc(i->height*3);
    c->length = i->height;

    int x, y;
    int step = i->width/20;
    for (y=0; y<i->height; y++) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (x=0; x<i->width; x+=step) {
            bsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            rsum += i->data[y*i->width*3+3*x+2];
        }
        c->data[3*y+0] = 1.0*rsum*step/i->width;
        c->data[3*y+1] = 1.0*gsum*step/i->width;
        c->data[3*y+2] = 1.0*bsum*step/i->width;
    }

    return c;
}

column* video_get_column(video *f, double min_percent, double max_percent) {
    image *i = get_frame(f, min_percent, max_percent);
    column *c = compress_to_column(i);
    free(i->data);
    free(i);
    return c;
}

void video_free(video *f) {
    avcodec_close(f->decoder_context);
    avformat_close_input(&f->format_context);
    avcodec_free_frame(&f->frame);
    free(f);
}
