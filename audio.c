#include "audio.h"
#include "error.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 8, 0)
AVFrame *av_frame_alloc(void) { return avcodec_alloc_frame(); }
void av_frame_free(AVFrame **frame) { av_freep(frame); }
#endif

struct audio {
    float start, end;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int audio_stream;
    AVFrame *frame;

    AVPacket packet;
    double time_base;
    double fps;
    long current_frame;
};

long audio_packet_pts(const audio *a, const AVPacket *packet) {
    long pts = packet->pts != 0 ? packet->pts : packet->dts;
    double sec = a->time_base*pts;
    return (int64_t)(a->fps*sec + 0.5);
}

int audio_grab_next_frame(audio *a) {
    int valid = 0;
    int got_frame = 0;

    double pts;

    while (!valid) {
        if (av_read_frame(a->format_context, &a->packet) >= 0) {
            if (a->packet.stream_index == a->audio_stream) {
                avcodec_decode_audio4(a->decoder_context, a->frame, &got_frame, &a->packet);
                if (got_frame) {
                    pts = audio_packet_pts(a, &a->packet);
                    valid = 1;
                }
            }
            av_free_packet(&a->packet);
        } else {
            av_free_packet(&a->packet);
            error("av_read_frame failed.");
            return 1;
        }
    }

    av_free_packet(&a->packet);
    a->current_frame = pts;
    return 0;
}

// TODO: av_frame_get_best_effort_timestamp

long audio_total_number_of_frames(const audio *a) {
    double duration_sec = 1.0*a->format_context->duration/AV_TIME_BASE;
    return a->fps*duration_sec;
}

audio* audio_init(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }

    av_log_set_level(AV_LOG_FATAL);
    av_register_all();

    audio *a;
    a = (audio *) malloc(sizeof(audio));
    a->start = 0.0;
    a->end = 1.0;
    a->format_context = NULL;

    if (avformat_open_input(&a->format_context, filename, NULL, NULL) != 0) {
        free(a);
        return NULL;
    }
    if (avformat_find_stream_info(a->format_context, NULL) < 0) {
        avformat_close_input(&a->format_context);
        return NULL;
    }
    a->audio_stream = av_find_best_stream(a->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (a->audio_stream == -1) {
        avformat_close_input(&a->format_context);
        return NULL;
    }
    a->decoder_context = a->format_context->streams[a->audio_stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(a->decoder_context->codec_id);
    if (codec == NULL) {
        error("Unsupported codec!");
        avformat_close_input(&a->format_context);
        return NULL;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(a->decoder_context, codec, &optionsDict) < 0) {
        avformat_close_input(&a->format_context);
        return NULL;
    }

    a->time_base = av_q2d(a->format_context->streams[a->audio_stream]->time_base);
    a->fps = a->decoder_context->sample_rate;

    a->frame = av_frame_alloc();
    a->current_frame = -1;

    if (audio_grab_next_frame(a) != 0) {
        return NULL;
    }
    return a;
}

int audio_seek(audio *a, const long min_frame_nr, const long max_frame_nr) {
    while (a->current_frame < min_frame_nr) {
        if (a->current_frame > max_frame_nr) {
            error("Target frame is in the past. This shoudn't happen. Please file a bug.");
        }
        if (audio_grab_next_frame(a) != 0) {
            return 1;
        }
    }
    return 0;
}

image* audio_get_frame(audio *a, const double min_percent, const double max_percent) {
    const long min_frame = min_percent*audio_total_number_of_frames(a);
    const long max_frame = max_percent*audio_total_number_of_frames(a);

    if (audio_seek(a, min_frame, max_frame) != 0) {
        return NULL;
    }

    image *frame;
    frame = (image *) malloc(sizeof(image));

    frame->width = 1;
    frame->height = 255;
    frame->data = (unsigned char *) malloc(frame->height*frame->width*3);

    float sum = 0;
    int i;
    for (i = 0; i < 1024; i++) {
        float d = *(((float *)a->frame->data[0])+i);
        //printf("smp: %f\n", d);
        sum += d*d/1024.0;
    }

    sum = sqrt(sum);
    sum *= 255*4;

    //printf("format: %d\n", a->frame->format);
    printf("%f\n", sum);
    if (sum > 255) {
        sum = 255;
    }

    int y;
    for (y = 0; y < sum; y++) {
        memset(frame->data+y*3+0, 255, 3);
    }
    for (y = sum; y < frame->height; y++) {
        memset(frame->data+y*3+0, 0, 3);
    }

    return frame;
}

float audio_start(const audio *a) {
    return a->start;
}

void audio_set_start(audio *a, const float start) {
    a->start = start;
}

float audio_end(const audio *a) {
    return a->end;
}

void audio_set_end(audio *a, const float end) {
    a->end = end;
}

void audio_free(audio *a) {
    avcodec_close(a->decoder_context);
    avformat_close_input(&a->format_context);
    av_frame_free(&a->frame);

    free(a);
}
