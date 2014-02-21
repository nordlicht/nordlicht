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

double fps(video *v) {
    return av_q2d(v->format_context->streams[v->video_stream]->r_frame_rate);
}

void grab_next_frame(video *v) {
    int valid = 0;
    int got_frame = 0;

    AVPacket packet;
    av_init_packet(&packet);

    double pts;

    while (!valid) {
        if (av_read_frame(v->format_context, &packet) >= 0) {
            if(packet.stream_index == v->video_stream) {
                avcodec_decode_video2(v->decoder_context, v->frame, &got_frame, &packet);
                if (got_frame) {
                    pts = packet.pts;
                    // to sec:
                    pts = pts*(double)av_q2d(v->format_context->streams[v->video_stream]->time_base);
                    // to frame number:
                    pts = pts*fps(v);
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

    v->current_frame = pts;
}

void seek_keyframe(video *v, long frame) {
    double time_base = av_q2d(v->format_context->streams[v->video_stream]->time_base);
    av_seek_frame(v->format_context, v->video_stream, frame/fps(v)/time_base, AVSEEK_FLAG_BACKWARD);
    grab_next_frame(v);
    avcodec_flush_buffers(v->decoder_context);
}

void seek_forward(video *v, long frame) {
    double time_base = av_q2d(v->format_context->streams[v->video_stream]->time_base);
    av_seek_frame(v->format_context, v->video_stream, frame/fps(v)/time_base, 0);
    grab_next_frame(v);
}

int total_number_of_frames(video *v) {
    double duration_sec = 1.0*v->format_context->duration/AV_TIME_BASE;
    return fps(v)*duration_sec;
}

void video_build_keyframe_index(video *v, int width) {
    v->keyframes = malloc(sizeof(long)*10*60*60*60); // TODO: dynamic datastructure!
    v->number_of_keyframes = 0;

    AVPacket packet;
    av_init_packet(&packet);
    int frame = 0;

    while (av_read_frame(v->format_context, &packet) >= 0) {
        if (packet.stream_index == v->video_stream) {
            if (!!(packet.flags & AV_PKT_FLAG_KEY)) {
                v->keyframes[++v->number_of_keyframes] = packet.pts;
            }
            frame++;
            if (!v->exact && frame == HEURISTIC_NUMBER_OF_FRAMES) {
                float density = 1.0*HEURISTIC_KEYFRAME_FACTOR*v->number_of_keyframes/HEURISTIC_NUMBER_OF_FRAMES;
                float required_density = 1.0*width/total_number_of_frames(v);
                if (density > required_density) {
                    // The keyframe density in the first `HEURISTIC_NUMBER_OF_FRAMES`
                    // frames is HEURISTIC_KEYFRAME_FACTOR times higher than
                    // the density we need overall.
                    printf("\rBuilding index: Enough keyframes (%.2f times enough), aborting.\n", density/required_density);
                    return;
                } else {
                    v->exact = 1;
                }
            }
        }
        av_free_packet(&packet);

        printf("\rBuilding index: %02.0f%%", 1.0*frame/total_number_of_frames(v)*100);
        fflush(stdout);
    }
    // TODO: Is it necessary to sort these?
    printf("\rBuilding index: %d keyframes.\n", v->number_of_keyframes);
}

video* video_init(char *filename, int exact, int width) {
    av_log_set_level(AV_LOG_FATAL);
    av_register_all();

    video *v;
    v = malloc(sizeof(video));
    v->exact = exact;
    v->format_context = NULL;

    if (avformat_open_input(&v->format_context, filename, NULL, NULL) != 0) {
        free(v);
        return NULL;
    }
    if (avformat_find_stream_info(v->format_context, NULL) < 0) {
        avformat_close_input(&v->format_context);
        return NULL;
    }
    v->video_stream = av_find_best_stream(v->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (v->video_stream == -1) {
        avformat_close_input(&v->format_context);
        return NULL;
    }
    v->decoder_context = v->format_context->streams[v->video_stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(v->decoder_context->codec_id);
    if (codec == NULL) {
        error("Unsupported codec!");
        avformat_close_input(&v->format_context);
        return NULL;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(v->decoder_context, codec, &optionsDict) < 0) {
        avformat_close_input(&v->format_context);
        return NULL;
    }

    v->frame = avcodec_alloc_frame();
    v->current_frame = -1;

    return v;
}

long preceding_keyframe(video *v, long frame_nr) {
    int i;
    long best_keyframe = -1;
    for (i=0; i < v->number_of_keyframes; i++) {
        if (v->keyframes[i] <= frame_nr) {
            best_keyframe = v->keyframes[i];
        }
    }
    return best_keyframe;
}

void seek(video *v, long min_frame_nr, long max_frame_nr) {
    if (v->exact) {
        long keyframe = preceding_keyframe(v, max_frame_nr);

        if (keyframe > v->current_frame) {
            seek_keyframe(v, keyframe);
        }

        while (v->current_frame < min_frame_nr) {
            if (v->current_frame > max_frame_nr) {
                // TODO: Can this happen?
            }
            grab_next_frame(v);
        }
    } else {
        seek_keyframe(v, (min_frame_nr+max_frame_nr)/2);
    }
}

image* get_frame(video *v, double min_percent, double max_percent) {
    seek(v, min_percent*total_number_of_frames(v), max_percent*total_number_of_frames(v));

    image *i;
    i = malloc(sizeof(image));

    i->width = v->frame->width;
    i->height = v->frame->height;
    i->data = malloc(i->height*i->width*3);

    AVFrame *frame;
    frame = avcodec_alloc_frame();
    frame->width = i->width;
    frame->height = i->height;
    frame->format = PIX_FMT_RGB24;

    uint8_t *buffer;
    buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, i->width, i->height));
    avpicture_fill((AVPicture *)frame, buffer, PIX_FMT_RGB24, i->width, i->height);

    struct SwsContext *sws_context = sws_getContext(v->frame->width, v->frame->height, v->frame->format,
            v->frame->width, v->frame->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);
    sws_scale(sws_context, (uint8_t const * const *)v->frame->data,
            v->frame->linesize, 0, v->frame->height, frame->data,
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

column* video_get_column(video *v, double min_percent, double max_percent, nordlicht_style s) {
    image *i = get_frame(v, min_percent, max_percent);
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

void video_free(video *v) {
    avcodec_close(v->decoder_context);
    avformat_close_input(&v->format_context);
    avcodec_free_frame(&v->frame);
    free(v);
}
