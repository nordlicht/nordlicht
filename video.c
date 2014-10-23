#include "video.h"
#include "error.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 8, 0)
AVFrame *av_frame_alloc(void) { return avcodec_alloc_frame(); }
void av_frame_free(AVFrame **frame) { av_freep(frame); }
#endif

#define HEURISTIC_NUMBER_OF_FRAMES 1800 // how many frames will the heuristic look at?
#define HEURISTIC_KEYFRAME_FACTOR 1 // lower bound for the actual/required keyframe ratio

struct video {
    int exact;
    float start, end;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
    AVFrame *frame;

    image *last_frame;

    uint8_t *buffer;
    AVFrame *scaleframe;
    struct SwsContext *sws_context;
    AVPacket packet;
    double time_base;
    double fps;

    long current_frame;
    int *keyframes;
    int number_of_keyframes;
    int has_index;
};

long packet_pts(const video *v, const AVPacket *packet) {
    long pts = packet->pts != 0 ? packet->pts : packet->dts;
    double sec = v->time_base*pts;
    return (int64_t)(v->fps*sec + 0.5);
}

int grab_next_frame(video *v) {
    int valid = 0;
    int got_frame = 0;

    double pts;

    while (!valid) {
        if (av_read_frame(v->format_context, &v->packet) >= 0) {
            if (v->packet.stream_index == v->video_stream) {
                avcodec_decode_video2(v->decoder_context, v->frame, &got_frame, &v->packet);
                if (got_frame) {
                    pts = packet_pts(v, &v->packet);
                    valid = 1;
                }
            }
            av_free_packet(&v->packet);
        } else {
            av_free_packet(&v->packet);
            error("av_read_frame failed.");
            v->current_frame = -1;
            return 1;
        }
    }

    av_free_packet(&v->packet);
    v->current_frame = pts;
    return 0;
}

void seek_keyframe(video *v, const long frame) {
    av_seek_frame(v->format_context, v->video_stream, frame/v->fps/v->time_base, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(v->decoder_context);
    grab_next_frame(v);
}

int total_number_of_frames(const video *v) {
    double duration_sec = 1.0*v->format_context->duration/AV_TIME_BASE;
    return v->fps*duration_sec;
}

void video_build_keyframe_index(video *v, const int width) {
    v->keyframes = (int *) malloc(sizeof(long)*60*60*3); // TODO: dynamic datastructure!
    v->number_of_keyframes = 0;

    int frame = 0;

    v->has_index = 0;
    v->exact = 1;
    seek_keyframe(v, 0);

    while (av_read_frame(v->format_context, &v->packet) >= 0) {
        if (v->packet.stream_index == v->video_stream) {
            if (!!(v->packet.flags & AV_PKT_FLAG_KEY)) {
                v->number_of_keyframes++;

                long pts = packet_pts(v, &v->packet);
                if (pts < 1 && v->number_of_keyframes > 0) {
                    pts = frame;
                }

                v->keyframes[v->number_of_keyframes] = pts;
            }
            if (frame == HEURISTIC_NUMBER_OF_FRAMES) {
                const float density = 1.0*v->number_of_keyframes/HEURISTIC_NUMBER_OF_FRAMES;
                const float required_density = 1.0*HEURISTIC_KEYFRAME_FACTOR/COLUMN_PRECISION*width/total_number_of_frames(v)/(v->end-v->start);
                if (density > required_density) {
                    // The keyframe density in the first `HEURISTIC_NUMBER_OF_FRAMES`
                    // frames is HEURISTIC_KEYFRAME_FACTOR times higher than
                    // the density we need overall.
                    v->exact = 0;
                    return;
                }
            }
            frame++;
        }
        av_free_packet(&v->packet);
    }
    v->has_index = 1;
}

video* video_init(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }

    av_log_set_level(AV_LOG_FATAL);
    av_register_all();

    video *v;
    v = (video *) malloc(sizeof(video));
    v->exact = 1;
    v->start = 0.0;
    v->end = 1.0;
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

    v->time_base = av_q2d(v->format_context->streams[v->video_stream]->time_base);
    v->fps = av_q2d(v->format_context->streams[v->video_stream]->avg_frame_rate);

    v->frame = av_frame_alloc();
    v->current_frame = -1;
    v->last_frame = NULL;
    v->has_index = 0;

    if (grab_next_frame(v) != 0) {
        return NULL;
    }

    v->scaleframe = av_frame_alloc();
    v->scaleframe->width = v->frame->width;
    v->scaleframe->height = v->frame->height;
    v->scaleframe->format = PIX_FMT_RGB24;

    v->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, v->scaleframe->width, v->scaleframe->height));
    avpicture_fill((AVPicture *)v->scaleframe, v->buffer, PIX_FMT_RGB24, v->frame->width, v->frame->height);

    v->sws_context = sws_getCachedContext(NULL, v->frame->width, v->frame->height, v->frame->format,
            v->scaleframe->width, v->scaleframe->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    return v;
}

long preceding_keyframe(video *v, const long frame_nr) {
    int i;
    long best_keyframe = -1;
    for (i = 0; i < v->number_of_keyframes; i++) {
        if (v->keyframes[i] <= frame_nr) {
            best_keyframe = v->keyframes[i];
        }
    }
    return best_keyframe;
}

int seek(video *v, const long min_frame_nr, const long max_frame_nr) {
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
    }
    return 0;
}

image* video_get_frame(video *v, const double min_percent, const double max_percent) {
    const long min_frame = min_percent*total_number_of_frames(v);
    const long max_frame = max_percent*total_number_of_frames(v);

    if (v->last_frame != NULL && !v->exact && v->has_index) {
        if (v->current_frame >= preceding_keyframe(v, (max_frame+min_frame)/2)) {
            return v->last_frame;
        }
    }

    if (seek(v, min_frame, max_frame) != 0) {
        return NULL;
    }

    if (v->last_frame == NULL) {
        v->last_frame = (image *) malloc(sizeof(image));

        v->last_frame->width = v->frame->width;
        v->last_frame->height = v->frame->height;
        v->last_frame->data = (unsigned char *) malloc(v->last_frame->height*v->last_frame->width*3);
    }

    sws_scale(v->sws_context, (uint8_t const * const *)v->frame->data,
            v->frame->linesize, 0, v->frame->height, v->scaleframe->data,
            v->scaleframe->linesize);

    int y;
    for (y = 0; y < v->last_frame->height; y++) {
        memcpy(v->last_frame->data+y*v->last_frame->width*3, v->scaleframe->data[0]+y*v->scaleframe->linesize[0], v->scaleframe->linesize[0]);
    }

    return v->last_frame;
}

int video_exact(const video *v) {
    return v->exact;
}

void video_set_exact(video *v, const int exact) {
    v->exact = exact;
    seek_keyframe(v, 0);
}

float video_start(const video *v) {
    return v->start;
}

void video_set_start(video *v, const float start) {
    v->start = start;
}

float video_end(const video *v) {
    return v->end;
}

void video_set_end(video *v, const float end) {
    v->end = end;
}

void video_free(video *v) {
    av_free(v->buffer);
    av_frame_free(&v->scaleframe);
    sws_freeContext(v->sws_context);

    avcodec_close(v->decoder_context);
    avformat_close_input(&v->format_context);
    av_frame_free(&v->frame);

    free(v->last_frame->data);
    free(v->last_frame);

    free(v->keyframes);

    free(v);
}
