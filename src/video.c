#include "video.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
#define avcodec_free_frame av_freep
#endif

#define HEURISTIC_NUMBER_OF_FRAMES 500
#define HEURISTIC_KEYFRAME_FACTOR 2

struct video {
    int exact;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
    AVFrame *frame;

    long current_frame;
    int *keyframes;
    int number_of_keyframes;
};

double fps(video *f) {
    return av_q2d(f->format_context->streams[f->video_stream]->r_frame_rate);
}

void grab_next_frame(video *f) {
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
            return;
        }
    }

    f->current_frame = pts;
}

void seek_keyframe(video *f, long frame) {
    double time_base = av_q2d(f->format_context->streams[f->video_stream]->time_base);
    av_seek_frame(f->format_context, f->video_stream, frame/fps(f)/time_base, AVSEEK_FLAG_BACKWARD);
    grab_next_frame(f);
}

void seek_forward(video *f, long frame) {
    double time_base = av_q2d(f->format_context->streams[f->video_stream]->time_base);
    av_seek_frame(f->format_context, f->video_stream, frame/fps(f)/time_base, 0);
    grab_next_frame(f);
}

int total_number_of_frames(video *f) {
    double duration_sec = 1.0*f->format_context->duration/AV_TIME_BASE;
    return fps(f)*duration_sec;
}

void build_keyframe_index(video *f, int width) {
    f->keyframes = malloc(sizeof(long)*10*60*60*60); // TODO: dynamic datastructure!
    f->number_of_keyframes = 0;

    AVPacket packet;
    av_init_packet(&packet);
    int frame = 0;

    while (av_read_frame(f->format_context, &packet) >= 0) {
        if (packet.stream_index == f->video_stream) {
            if (!!(packet.flags & AV_PKT_FLAG_KEY)) {
                f->keyframes[++f->number_of_keyframes] = packet.pts;
            }
            frame++;
            if (!f->exact && frame == HEURISTIC_NUMBER_OF_FRAMES) {
                float density = 1.0*HEURISTIC_KEYFRAME_FACTOR*f->number_of_keyframes/HEURISTIC_NUMBER_OF_FRAMES;
                float required_density = 1.0*width/total_number_of_frames(f);
                if (density > required_density) {
                    // The keyframe density in the first `HEURISTIC_NUMBER_OF_FRAMES`
                    // frames is HEURISTIC_KEYFRAME_FACTOR times higher than
                    // the density we need overall.
                    printf("\rBuilding index: Enough keyframes (%.2f times enough), aborting.\n", density/required_density);
                    return;
                } else {
                    f->exact = 1;
                }
            }
        }
        av_free_packet(&packet);

        printf("\rBuilding index: %02.0f%%", 1.0*frame/total_number_of_frames(f)*100);
        fflush(stdout);
    }
    // TODO: Is it necessary to sort these?
    printf("\rBuilding index: %d keyframes.\n", f->number_of_keyframes);
}

video* video_init(char *filename, int exact, int width) {
    av_log_set_level(AV_LOG_FATAL);
    av_register_all();

    video *f;
    f = malloc(sizeof(video));
    f->exact = exact;
    f->format_context = NULL;

    if (avformat_open_input(&f->format_context, filename, NULL, NULL) != 0) {
        free(f);
        return NULL;
    }
    if (avformat_find_stream_info(f->format_context, NULL) < 0) {
        avformat_close_input(&f->format_context);
        return NULL;
    }
    f->video_stream = av_find_best_stream(f->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (f->video_stream == -1) {
        avformat_close_input(&f->format_context);
        return NULL;
    }
    f->decoder_context = f->format_context->streams[f->video_stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(f->decoder_context->codec_id);
    if (codec == NULL) {
        error("Unsupported codec!");
        avformat_close_input(&f->format_context);
        return NULL;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(f->decoder_context, codec, &optionsDict) < 0) {
        avformat_close_input(&f->format_context);
        return NULL;
    }

    f->frame = avcodec_alloc_frame();
    f->current_frame = -1;

    return f;
}

long preceding_keyframe(video *f, long frame_nr) {
    int i;
    long best_keyframe = -1;
    for (i=0; i < f->number_of_keyframes; i++) {
        if (f->keyframes[i] <= frame_nr) {
            best_keyframe = f->keyframes[i];
        }
    }
    return best_keyframe;
}

void seek(video *f, long min_frame_nr, long max_frame_nr) {
    if (f->exact) {
        long keyframe = preceding_keyframe(f, max_frame_nr);

        if (keyframe > f->current_frame) {
            seek_keyframe(f, keyframe);
        }

        while (f->current_frame < min_frame_nr) {
            if (f->current_frame > max_frame_nr) {
                // TODO: Can this happen?
            }
            grab_next_frame(f);
        }
    } else {
        seek_keyframe(f, (min_frame_nr+max_frame_nr)/2);
    }
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

column* compress_to_row(image *i) {
    column *c;
    c = malloc(sizeof(column));
    c->data = malloc(i->width*3);
    c->length = i->width;

    int x, y;
    int step = i->height/20;
    for (x=i->width-1; x>=0; x--) {
        long rsum = 0;
        long gsum = 0;
        long bsum = 0;
        for (y=0; y<i->height; y+=step) {
            bsum += i->data[y*i->width*3+3*x+0];
            gsum += i->data[y*i->width*3+3*x+1];
            rsum += i->data[y*i->width*3+3*x+2];
        }
        c->data[3*x+0] = 1.0*rsum*step/i->height;
        c->data[3*x+1] = 1.0*gsum*step/i->height;
        c->data[3*x+2] = 1.0*bsum*step/i->height;
    }

    return c;
}

column* compress_to_diagonal(image *i) {
    column *c;
    c = malloc(sizeof(column));
    c->data = malloc(i->width*3);
    c->length = i->width;

    float slope = 1.0*i->height/i->width;

    int x;
    for (x=0; x<i->width; x++) {
        int y = x*slope;
        c->data[3*x+0] = i->data[y*i->width*3+3*x+2];
        c->data[3*x+1] = i->data[y*i->width*3+3*x+1];
        c->data[3*x+2] = i->data[y*i->width*3+3*x+0];
    }

    return c;
}

column* video_get_column(video *f, double min_percent, double max_percent, nordlicht_style s) {
    image *i = get_frame(f, min_percent, max_percent);
    column *c;

    switch (s) {
        case NORDLICHT_STYLE_HORIZONTAL:
            c = compress_to_column(i);
            break;
        case NORDLICHT_STYLE_VERTICAL:
            c = compress_to_row(i);
            break;
    }
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
