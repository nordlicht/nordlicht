#include "video.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
#define avcodec_free_frame av_freep
#endif

#define HEURISTIC_NUMBER_OF_FRAMES 1800 // how many frames will the heuristic look at?
#define HEURISTIC_KEYFRAME_FACTOR 2 // lower bound for the actual/required keyframe ratio

struct video {
    int exact;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
    AVFrame *frame;

    uint8_t *buffer;
    AVFrame *scaleframe;
    struct SwsContext *sws_context;

    long current_frame;
    int *keyframes;
    int number_of_keyframes;
};

double fps(video *v) {
    return av_q2d(v->format_context->streams[v->video_stream]->r_frame_rate);
}

int grab_next_frame(video *v) {
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
            error("av_read_frame failed.");
            v->current_frame = -1;
            return 1;
        }
    }

    v->current_frame = pts;
    return 0;
}

void seek_keyframe(video *v, long frame) {
    double time_base = av_q2d(v->format_context->streams[v->video_stream]->time_base);
    av_seek_frame(v->format_context, v->video_stream, frame/fps(v)/time_base, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(v->decoder_context);
    grab_next_frame(v);
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
            if (frame == HEURISTIC_NUMBER_OF_FRAMES) {
                float density = 1.0*v->number_of_keyframes/HEURISTIC_NUMBER_OF_FRAMES;
                float required_density = 1.0*HEURISTIC_KEYFRAME_FACTOR/COLUMN_PRECISION*width/total_number_of_frames(v);
                if (density > required_density) {
                    // The keyframe density in the first `HEURISTIC_NUMBER_OF_FRAMES`
                    // frames is HEURISTIC_KEYFRAME_FACTOR times higher than
                    // the density we need overall.
                    printf("\rBuilding index: Enough keyframes (%.2f times enough), aborting.\n", density/required_density);
                    v->exact = 0;
                    return;
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

video* video_init(char *filename, int width) {
    av_log_set_level(AV_LOG_FATAL);
    av_register_all();

    video *v;
    v = malloc(sizeof(video));
    v->exact = 1;
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

    if (grab_next_frame(v) != 0) {
        error("fuck");
        exit(1);
    }

    v->scaleframe = avcodec_alloc_frame();
    v->scaleframe->width = v->frame->width;
    v->scaleframe->height = v->frame->height;
    v->scaleframe->format = PIX_FMT_RGB24;

    v->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, v->scaleframe->width, v->scaleframe->height));
    printf("yay\n");
    avpicture_fill((AVPicture *)v->scaleframe, v->buffer, PIX_FMT_RGB24, v->frame->width, v->frame->height);

    v->sws_context = sws_getContext(v->frame->width, v->frame->height, v->frame->format,
            v->scaleframe->width, v->scaleframe->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

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

int seek(video *v, long min_frame_nr, long max_frame_nr) {
    if (v->exact) {
        long keyframe = preceding_keyframe(v, max_frame_nr);

        if (keyframe > v->current_frame) {
            seek_keyframe(v, keyframe);
        }

        while (v->current_frame < min_frame_nr) {
            if (v->current_frame > max_frame_nr) {
                error("Target frame is in the past. This shoudn't happen. Please file a bug.");
            }
            if (grab_next_frame(v) != 0) {
                return 1;
            }
        }
    } else {
        seek_keyframe(v, (min_frame_nr+max_frame_nr)/2);
        if (v->current_frame < min_frame_nr || v->current_frame > max_frame_nr) {
            error("Our heuristic failed: %ld is not between %ld and %ld.", v->current_frame, min_frame_nr, max_frame_nr);
        }
    }
    return 0;
}

image* get_frame(video *v, double min_percent, double max_percent) {
    if (seek(v, min_percent*total_number_of_frames(v), max_percent*total_number_of_frames(v)) != 0) {
        return NULL;
    }

    image *i;
    i = malloc(sizeof(image));

    i->width = v->frame->width;
    i->height = v->frame->height;
    i->data = malloc(i->height*i->width*3);

    sws_scale(v->sws_context, (uint8_t const * const *)v->frame->data,
            v->frame->linesize, 0, v->frame->height, v->scaleframe->data,
            v->scaleframe->linesize);

    int y;
    for (y=0; y<i->height; y++) {
        memcpy(i->data+y*i->width*3, v->scaleframe->data[0]+y*v->scaleframe->linesize[0], v->scaleframe->linesize[0]);
    }

    return i;
}

column* video_get_column(video *v, double min_percent, double max_percent, nordlicht_style s) {
    image *i = get_frame(v, min_percent, max_percent);

    if (i == NULL) {
        return NULL;
    }

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
    av_free(v->buffer);
    avcodec_free_frame(&v->scaleframe);
    sws_freeContext(v->sws_context);

    avcodec_close(v->decoder_context);
    avformat_close_input(&v->format_context);
    avcodec_free_frame(&v->frame);
    free(v);
}
