#include "source.h"
#include "error.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <fftw3.h>

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 8, 0)
AVFrame *av_frame_alloc(void) { return avcodec_alloc_frame(); }
void av_frame_free(AVFrame **frame) { av_freep(frame); }
#endif

#define HEURISTIC_NUMBER_OF_FRAMES 1800 // how many frames will the heuristic look at?
#define HEURISTIC_KEYFRAME_FACTOR 1 // lower bound for the actual/required keyframe ratio

#define SAMPLES_PER_FRAME 1024

typedef struct {
    int stream;
    AVCodecContext *codec;
    image *last_frame;

    double time_base;
    double fps;
    AVFrame *frame;
    long current_frame;
} stream;

struct source {
    int exact;
    float start, end;

    AVFormatContext *format;
    stream *video;
    stream *audio;

    uint8_t *buffer;
    AVFrame *scaleframe;
    struct SwsContext *sws_context;
    AVPacket packet;

    int *keyframes;
    int number_of_keyframes;
    int has_index;
};

long packet_pts(stream *st, const AVPacket *packet) {
    long pts = packet->pts != 0 ? packet->pts : packet->dts;
    double sec = st->time_base*pts;
    return (int64_t)(st->fps*sec + 0.5);
}

int grab_next_frame(source *s, stream *st) {
    int valid = 0;
    int got_frame = 0;

    long pts;

    while (!valid) {
        if (av_read_frame(s->format, &s->packet) >= 0) {
            if (s->packet.stream_index == st->stream) {
                switch (st->codec->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        avcodec_decode_video2(st->codec, st->frame, &got_frame, &s->packet);
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        avcodec_decode_audio4(st->codec, st->frame, &got_frame, &s->packet);
                        break;
                    default:
                        error("Stream has unknown media type.");
                        return 1;
                }

                if (got_frame) {
                    pts = packet_pts(st, &s->packet);
                    valid = 1;
                }
            }
            av_free_packet(&s->packet);
        } else {
            av_free_packet(&s->packet);
            error("av_read_frame failed.");
            st->current_frame = -1;
            return 1;
        }
    }

    av_free_packet(&s->packet);
    st->current_frame = pts;
    return 0;
}

void seek_keyframe(source *s, stream *st, const long frame) {
    av_seek_frame(s->format, st->stream, frame/st->fps/st->time_base, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(st->codec);
    grab_next_frame(s, st);
}

int total_number_of_frames(const source *s, stream *st) {
    double duration_sec = 1.0*s->format->duration/AV_TIME_BASE;
    return st->fps*duration_sec;
}

void source_build_keyframe_index(source *s, const int width) {
    s->keyframes = (int *) malloc(sizeof(long)*60*60*3); // TODO: dynamic datastructure!
    s->number_of_keyframes = 0;

    int frame = 0;

    s->has_index = 0;
    s->exact = 1;
    seek_keyframe(s, s->video, 0);

    while (av_read_frame(s->format, &s->packet) >= 0) {
        if (s->packet.stream_index == s->video->stream) {
            if (!!(s->packet.flags & AV_PKT_FLAG_KEY)) {
                s->number_of_keyframes++;

                long pts = packet_pts(s->video, &s->packet);
                if (pts < 1 && s->number_of_keyframes > 0) {
                    pts = frame;
                }

                s->keyframes[s->number_of_keyframes] = pts;
            }
            if (frame == HEURISTIC_NUMBER_OF_FRAMES) {
                const float density = 1.0*s->number_of_keyframes/HEURISTIC_NUMBER_OF_FRAMES;
                const float required_density = 1.0*HEURISTIC_KEYFRAME_FACTOR/COLUMN_PRECISION*width/total_number_of_frames(s, s->video)/(s->end-s->start);
                if (density > required_density) {
                    // The keyframe density in the first `HEURISTIC_NUMBER_OF_FRAMES`
                    // frames is HEURISTIC_KEYFRAME_FACTOR times higher than
                    // the density we need overall.
                    s->exact = 0;
                    return;
                }
            }
            frame++;
        }
        av_free_packet(&s->packet);
    }
    s->has_index = 1;
}

stream* stream_init(source *s, enum AVMediaType type) {
    stream *st;
    st = (stream *) malloc(sizeof(stream));

    st->last_frame = NULL;

    st->stream = av_find_best_stream(s->format, type, -1, -1, NULL, 0);
    if (st->stream == -1) {
        avformat_close_input(&s->format);
        return NULL;
    }
    st->codec = s->format->streams[st->stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(st->codec->codec_id);
    if (codec == NULL) {
        error("Unsupported codec!");
        avformat_close_input(&s->format);
        return NULL;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(st->codec, codec, &optionsDict) < 0) {
        avformat_close_input(&s->format);
        return NULL;
    }

    st->time_base = av_q2d(s->format->streams[st->stream]->time_base);

    switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            st->fps = av_q2d(s->format->streams[st->stream]->avg_frame_rate);
            break;
        case AVMEDIA_TYPE_AUDIO:
            st->fps = st->codec->sample_rate;
            break;
        default:
            error("Stream has unknown media type.");
            return NULL;
    }

    st->frame = av_frame_alloc();
    st->current_frame = -1;

    if (grab_next_frame(s, st) != 0) {
        return NULL;
    }

    return st;
}

source* source_init(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }

    av_log_set_level(AV_LOG_FATAL);
    av_register_all();

    source *s;
    s = (source *) malloc(sizeof(source));
    s->exact = 1;
    s->start = 0.0;
    s->end = 1.0;
    s->format = NULL;

    if (avformat_open_input(&s->format, filename, NULL, NULL) != 0) {
        free(s);
        return NULL;
    }
    if (avformat_find_stream_info(s->format, NULL) < 0) {
        avformat_close_input(&s->format);
        return NULL;
    }

    s->video = stream_init(s, AVMEDIA_TYPE_VIDEO);
    s->audio = stream_init(s, AVMEDIA_TYPE_AUDIO);

    s->has_index = 0;

    s->scaleframe = av_frame_alloc();
    s->scaleframe->width = s->video->frame->width;
    s->scaleframe->height = s->video->frame->height;
    s->scaleframe->format = PIX_FMT_RGB24;

    s->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, s->scaleframe->width, s->scaleframe->height));
    avpicture_fill((AVPicture *)s->scaleframe, s->buffer, PIX_FMT_RGB24, s->video->frame->width, s->video->frame->height);

    s->sws_context = sws_getCachedContext(NULL, s->video->frame->width, s->video->frame->height, s->video->frame->format,
            s->scaleframe->width, s->scaleframe->height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    return s;
}

long preceding_keyframe(source *s, const long frame_nr) {
    int i;
    long best_keyframe = -1;
    for (i = 0; i < s->number_of_keyframes; i++) {
        if (s->keyframes[i] <= frame_nr) {
            best_keyframe = s->keyframes[i];
        }
    }
    return best_keyframe;
}

int seek(source *s, stream *st, const long min_frame_nr, const long max_frame_nr) {
    if (s->exact && st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        long keyframe = preceding_keyframe(s, max_frame_nr);

        if (keyframe > st->current_frame) {
            seek_keyframe(s, st, keyframe);
        }

        while (st->current_frame < min_frame_nr) {
            if (st->current_frame > max_frame_nr) {
                error("Target frame is in the past. This shoudn't happen. Please file a bug.");
            }
            if (grab_next_frame(s, st) != 0) {
                return 1;
            }
        }
    } else {
        seek_keyframe(s, st, (min_frame_nr+max_frame_nr)/2);
    }
    return 0;
}

image* source_get_video_frame(source *s, const double min_percent, const double max_percent) {
    float proportion = s->end-s->start;
    const long min_frame = (min_percent*proportion + s->start)*total_number_of_frames(s, s->video);
    const long max_frame = (max_percent*proportion + s->start)*total_number_of_frames(s, s->video);

    if (s->video->last_frame != NULL && !s->exact && s->has_index) {
        if (s->video->current_frame >= preceding_keyframe(s, (max_frame+min_frame)/2)) {
            return s->video->last_frame;
        }
    }

    if (seek(s, s->video, min_frame, max_frame) != 0) {
        return NULL;
    }

    if (s->video->last_frame == NULL) {
        s->video->last_frame = image_init(s->video->frame->width, s->video->frame->height);
    }

    sws_scale(s->sws_context, (uint8_t const * const *)s->video->frame->data,
            s->video->frame->linesize, 0, s->video->frame->height, s->scaleframe->data,
            s->scaleframe->linesize);

    image_copy_avframe(s->video->last_frame, s->scaleframe);

    return s->video->last_frame;
}

int colormap_r(float dbfs) {
    if (dbfs >= -30) return 255;
    if (dbfs <= -40) return 0;
    return (dbfs+40)/10.0*255;
}

int colormap_g(float dbfs) {
    if (dbfs >= -10) return (dbfs+10)/10.0*255;
    if (dbfs < -20 && dbfs >= -30) return 255-(dbfs+30)/10.0*255;
    if (dbfs < -30 && dbfs >= -50) return 255;
    if (dbfs < -50 && dbfs >= -60) return (dbfs+60)/10.0*255;
    return 0;
}

int colormap_b(float dbfs) {
    if (dbfs >= -10) return 255;
    if (dbfs < -10 && dbfs >= -20) return (dbfs+20)/10.0*255;
    if (dbfs < -40 && dbfs >= -50) return 255-(dbfs+50)/10.0*255;
    if (dbfs < -50 && dbfs >= -60) return 255;
    if (dbfs < -60 && dbfs >= -70) return (dbfs+70)/10.0*255;
    return 0;
}

image* source_get_audio_frame(source *s, const double min_percent, const double max_percent) {
    float proportion = s->end-s->start;
    const long min_frame = (min_percent*proportion + s->start)*total_number_of_frames(s, s->audio);
    const long max_frame = (max_percent*proportion + s->start)*total_number_of_frames(s, s->audio);

    if (s->audio->last_frame != NULL && !s->exact && s->has_index) {
        if (s->audio->current_frame >= preceding_keyframe(s, (max_frame+min_frame)/2)) {
            return s->audio->last_frame;
        }
    }

    if (seek(s, s->audio, min_frame, max_frame) != 0) {
        return NULL;
    }

    if (s->audio->last_frame == NULL) {
        s->audio->last_frame = image_init(1, 100);
    }

    fftw_complex *out;
    fftw_plan p;
    double *in = (double*) fftw_malloc(sizeof(double)*SAMPLES_PER_FRAME);
    out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex)*(SAMPLES_PER_FRAME/2+1));
    p = fftw_plan_dft_r2c_1d(SAMPLES_PER_FRAME, in, out, FFTW_ESTIMATE);

    int i;
    for (i = 0; i < SAMPLES_PER_FRAME; i++) {
        double v = *(((float *)s->audio->frame->data[0])+i);
        in[i] = v;
    }

    fftw_execute(p);

    int f;
    for (f = 0; f < 100; f++) {
        double absval = out[f][0]*out[f][0]+out[f][1]*out[f][1];
        double db = 10*log10(absval/10000.0); // TODO: finetune
        image_set(s->audio->last_frame, 0, 100-f-1, colormap_r(db), colormap_g(db), colormap_b(db));
    }

    return s->audio->last_frame;
}

image* source_get_audio_frame2(source *s, const double min_percent, const double max_percent) {
    float proportion = s->end-s->start;
    const long min_frame = (min_percent*proportion + s->start)*total_number_of_frames(s, s->audio);
    const long max_frame = (max_percent*proportion + s->start)*total_number_of_frames(s, s->audio);

    if (s->audio->last_frame != NULL && !s->exact && s->has_index) {
        if (s->audio->current_frame >= preceding_keyframe(s, (max_frame+min_frame)/2)) {
            return s->audio->last_frame;
        }
    }

    if (seek(s, s->audio, min_frame, max_frame) != 0) {
        return NULL;
    }

    if (s->audio->last_frame == NULL) {
        s->audio->last_frame = image_init(1, 100);
    }

    float sum = 0;
    int i;
    for (i = 0; i < SAMPLES_PER_FRAME; i++) {
        float d = *(((float *)s->audio->frame->data[0])+i);
        sum += 1.0*d*d/SAMPLES_PER_FRAME;
    }

    if (sum < 0) {
        sum = 0;
    }

    sum = sqrt(sum);
    sum *= 255*4;

    if (sum > 255) {
        sum = 255;
    }

    int y;
    for (y = 0; y < image_height(s->audio->last_frame); y++) {
        if (y >= image_height(s->audio->last_frame)-sum) {
            image_set(s->audio->last_frame, 0, y, 255, 255, 255);
        } else {
            image_set(s->audio->last_frame, 0, y, 0, 0, 0);
        }
    }

    return s->audio->last_frame;
}

int source_exact(const source *s) {
    return s->exact;
}

void source_set_exact(source *s, const int exact) {
    s->exact = exact;
    s->video->current_frame = -1;
    s->audio->current_frame = -1;
}

float source_start(const source *s) {
    return s->start;
}

void source_set_start(source *s, const float start) {
    s->start = start;
}

float source_end(const source *s) {
    return s->end;
}

void source_set_end(source *s, const float end) {
    s->end = end;
}

void stream_free(stream *st) {
    avcodec_close(st->codec);

    if (st->last_frame != NULL) {
        image_free(st->last_frame);
    }

    av_frame_free(&st->frame);

    free(st);
}

void source_free(source *s) {
    av_free(s->buffer);
    av_frame_free(&s->scaleframe);
    sws_freeContext(s->sws_context);

    stream_free(s->video);
    stream_free(s->audio);

    avformat_close_input(&s->format);

    free(s->keyframes);

    free(s);
}
