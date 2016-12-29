#include "source.h"
#include "error.h"
#include <libavcodec/avcodec.h>
#include <libavcodec/avfft.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

// Changes for ffmpeg 3.0
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,0)
#  include <libavutil/imgutils.h>
#  define av_free_packet av_packet_unref
#  define avpicture_get_size(fmt,w,h) av_image_get_buffer_size(fmt,w,h,1)
#endif

// PIX_FMT was renamed to AV_PIX_FMT on this version
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51,74,100)
#  define AVPixelFormat PixelFormat
#  define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
#  define AV_PIX_FMT_YUVJ420P PIX_FMT_YUVJ420P
#  define AV_PIX_FMT_YUVJ422P PIX_FMT_YUVJ422P
#  define AV_PIX_FMT_YUVJ440P PIX_FMT_YUVJ440P
#  define AV_PIX_FMT_YUVJ444P PIX_FMT_YUVJ444P
#  define AV_PIX_FMT_YUV420P  PIX_FMT_YUV420P
#  define AV_PIX_FMT_YUV422P  PIX_FMT_YUV422P
#  define AV_PIX_FMT_YUV440P  PIX_FMT_YUV440P
#  define AV_PIX_FMT_YUV444P  PIX_FMT_YUV444P
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 8, 0)
#  define av_frame_alloc  avcodec_alloc_frame
#  if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54,59,100)
#    define av_frame_free   av_freep
#  else
#    define av_frame_free   avcodec_free_frame
#  endif
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

    // audio specific
    RDFTContext *rdft;

    int *keyframes;
    int number_of_keyframes;
    int has_index;
    int current_frame;
};

long packet_pts(stream *st, const AVPacket *packet) {
    long pts = packet->pts != 0 ? packet->pts : packet->dts;
    double sec = st->time_base*pts;
    return (int64_t)(st->fps*sec + 0.5);
}

int grab_next_frame(source *s, stream *st) {
#ifdef DEBUG
    printf("gnf called\n");
#endif
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
#ifdef DEBUG
    printf("Unknown media type?\n");
#endif
                        return 1;
                }

                if (got_frame) {
                    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                        if (st->frame->data[0] == 0) {
#ifdef DEBUG
    printf("grab_next_frame: st->frame->data[0] == 0\n");
#endif
                            return 1;
                        }
                    }
                    pts = packet_pts(st, &s->packet);
                    valid = 1;
                }
            }
            av_free_packet(&s->packet);
        } else {
            av_free_packet(&s->packet);
            st->current_frame = -1;
#ifdef DEBUG
    printf("grab_next_frame: av_read_frame() == 0\n");
#endif
            return 1;
        }
    }

    av_free_packet(&s->packet);
    st->current_frame = pts;
#ifdef DEBUG
    printf("grab_next_frame() grabbed frame %d\n", pts);
#endif
    return 0;
}

int seek_keyframe(source *s, stream *st, const long frame) {
#ifdef DEBUG
    printf("seek_keyframe(%ld)\n", frame);
#endif
    av_seek_frame(s->format, st->stream, frame/st->fps/st->time_base, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(st->codec);
    return grab_next_frame(s, st) != 0;
}

int total_number_of_frames(const source *s, stream *st) {
    double duration_sec = 1.0*s->format->duration/AV_TIME_BASE;
    return st->fps*duration_sec;
}

// Returns 0 if done
int source_build_keyframe_index_step(source *s, const int width) {
    if (s->keyframes == NULL) {
        if (! s->video) {
            return 0;
        }
        s->keyframes = (int *) malloc(sizeof(long)*60*60*3); // TODO: dynamic datastructure!
        s->number_of_keyframes = 0;

        s->current_frame = 0;

        s->has_index = 0;
        s->exact = 1;
    }

    if (av_read_frame(s->format, &s->packet) >= 0) {
        if (s->packet.stream_index == s->video->stream) {
            if (!!(s->packet.flags & AV_PKT_FLAG_KEY)) {
                s->number_of_keyframes++;

                long pts = packet_pts(s->video, &s->packet);
                if (pts < 1 && s->number_of_keyframes > 0) {
                    pts = s->current_frame;
                }

#ifdef DEBUG
        printf("Found a keyframe: %ld\n", pts);
#endif
                s->keyframes[s->number_of_keyframes] = pts;
            }
            if (s->current_frame == HEURISTIC_NUMBER_OF_FRAMES) {
                const float density = 1.0*s->number_of_keyframes/HEURISTIC_NUMBER_OF_FRAMES;
                const float required_density = 1.0*HEURISTIC_KEYFRAME_FACTOR/COLUMN_PRECISION*width/total_number_of_frames(s, s->video)/(s->end-s->start);
                if (density > required_density) {
                    // The keyframe density in the first `HEURISTIC_NUMBER_OF_FRAMES`
                    // frames is HEURISTIC_KEYFRAME_FACTOR times higher than
                    // the density we need overall. This means that we can abort build
                    // the keyframe index and just seek inexactly to get good results.
                    av_free_packet(&s->packet);
#ifdef DEBUG
        printf("Keyframe indexing not necessary. Aborting.\n");
#endif
                    return 0;
                }
            }
            s->current_frame++;
        }
        av_free_packet(&s->packet);
        return 1;
    } else {
        // we read through the whole file
        av_free_packet(&s->packet);
        s->has_index = 1;
#ifdef DEBUG
    printf("Keyframe indexing complete.\n");
#endif
        return 0;
    }
}

stream* stream_init(source *s, enum AVMediaType type) {
    stream *st;
    st = (stream *) malloc(sizeof(stream));

    st->last_frame = NULL;

    st->stream = av_find_best_stream(s->format, type, -1, -1, NULL, 0);
    if (st->stream < 0) {
        free(st);
        return NULL;
    }
    st->codec = s->format->streams[st->stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(st->codec->codec_id);
    if (codec == NULL) {
        error("Unsupported codec!");
        free(st);
        return NULL;
    }
    if (avcodec_open2(st->codec, codec, NULL) < 0) {
        free(st);
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
            free(st);
            return NULL;
    }

    st->frame = av_frame_alloc();
    st->current_frame = -1;

    if (grab_next_frame(s, st) != 0) {
        free(st);
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
    if (strstr(filename, "://") != 0) {
      avformat_network_init();
    }

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
        free(s);
        return NULL;
    }

    s->video = stream_init(s, AVMEDIA_TYPE_VIDEO);
    s->audio = stream_init(s, AVMEDIA_TYPE_AUDIO);

    if (!s->video && !s->audio) {
        error("File contains neither video nor audio");
        avformat_close_input(&s->format);
        free(s);
        return NULL;
    }

    s->has_index = 0;

    if (s->video) {
        s->scaleframe = av_frame_alloc();
        s->scaleframe->width = s->video->frame->width;
        s->scaleframe->height = s->video->frame->height;
        s->scaleframe->format = AV_PIX_FMT_RGB24;

        s->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(s->scaleframe->format, s->scaleframe->width, s->scaleframe->height));
        #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,0)
            av_image_fill_arrays(s->scaleframe->data, s->scaleframe->linesize, s->buffer, s->scaleframe->format, s->video->frame->width, s->video->frame->height, 1);
        #else
            avpicture_fill((AVPicture *)s->scaleframe, s->buffer, s->scaleframe->format, s->video->frame->width, s->video->frame->height);
        #endif

        s->sws_context = sws_getCachedContext(NULL, s->video->frame->width, s->video->frame->height, s->video->frame->format,
                s->scaleframe->width, s->scaleframe->height, s->scaleframe->format, SWS_AREA, NULL, NULL, NULL);
    }

    s->keyframes = NULL;

    // audio specific
    if (s->audio) {
        s->rdft = av_rdft_init(log2(SAMPLES_PER_FRAME), DFT_R2C);
    }

    return s;
}

int source_has_video(source *s) {
    return s->video != NULL;
}

int source_has_audio(source *s) {
    return s->audio != NULL;
}

long preceding_keyframe(source *s, const long frame_nr) {
    int i;
    long best_keyframe = -1;
    for (i = 0; i < s->number_of_keyframes; i++) {
        if (s->keyframes[i] <= frame_nr) {
            best_keyframe = s->keyframes[i];
        }
    }
#ifdef DEBUG
    printf("preceding_keyframe(%ld) -> %ld\n", frame_nr, best_keyframe);
#endif
    return best_keyframe;
}

int seek(source *s, stream *st, const long min_frame_nr, const long max_frame_nr) {
#ifdef DEBUG
    printf("seek(%ld, %ld)\n", min_frame_nr, max_frame_nr);
#endif
    if (s->exact && st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        long keyframe = preceding_keyframe(s, max_frame_nr);

        if (keyframe > st->current_frame) {
            if (seek_keyframe(s, st, keyframe) != 0) {
#ifdef DEBUG
    printf("seek_keyframe() == 0\n");
#endif
                return 1;
            }
        }

        while (st->current_frame < min_frame_nr) {
            if (st->current_frame > max_frame_nr) {
                error("Target frame is in the past. This shouldn't happen. Please file a bug.");
            }
            if (grab_next_frame(s, st) != 0) {
#ifdef DEBUG
    printf("grab_next_frame() == 0\n");
#endif
                return 1;
            }
        }
    } else {
        if (seek_keyframe(s, st, (min_frame_nr+max_frame_nr)/2) != 0) {
#ifdef DEBUG
    printf("seek_keyframe() == 0\n");
#endif
            return 1;
        }
    }
    return 0;
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

image* get_frame(source *s, stream *st, const double min_percent, const double max_percent) {
    float proportion = s->end-s->start;
    const long min_frame = (min_percent*proportion + s->start)*total_number_of_frames(s, st);
    const long max_frame = (max_percent*proportion + s->start)*total_number_of_frames(s, st);

    if (st->last_frame != NULL && !s->exact && s->has_index) {
        if (st->current_frame >= preceding_keyframe(s, (max_frame+min_frame)/2)) {
            return st->last_frame;
        }
    }

    if (seek(s, st, min_frame, max_frame) != 0) {
        return NULL;
    }

    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (st->last_frame == NULL) {
            st->last_frame = image_init(st->frame->width, st->frame->height);
        }

        sws_scale(s->sws_context, (uint8_t const * const *)st->frame->data,
                st->frame->linesize, 0, st->frame->height, s->scaleframe->data,
                s->scaleframe->linesize);

        image_copy_avframe(st->last_frame, s->scaleframe);
    } else {
        if (s->audio->last_frame == NULL) {
            s->audio->last_frame = image_init(1, 100);
        }

        float *data;
        int i;
        switch(st->codec->sample_fmt) {
            case AV_SAMPLE_FMT_FLTP:
                data = (float *) s->audio->frame->data[0];
                break;
            case AV_SAMPLE_FMT_S16P:
                data = (float *) malloc(sizeof(float)*SAMPLES_PER_FRAME);
                for (i = 0; i < SAMPLES_PER_FRAME; i++) {
                    float val = ((int16_t *) s->audio->frame->data[0])[i]/100000.0;
                    data[i] = val;
                }

                break;
            default:
                error("Unsupported sample format (%d), see https://github.com/nordlicht/nordlicht/issues/26", st->codec->sample_fmt);
                return NULL;
        }

        av_rdft_calc(s->rdft, data);

        int f;
        for (f = 0; f < 100; f++) {
            double absval = data[2*f+0]*data[2*f+0]+data[2*f+1]*data[2*f+1];
            double dbfs = 10*log10(absval/10000.0); // TODO: finetune
            image_set(s->audio->last_frame, 0, 100-f-1, colormap_r(dbfs), colormap_g(dbfs), colormap_b(dbfs));
        }
    }

    return st->last_frame;
}

image* source_get_video_frame(source *s, const double min_percent, const double max_percent) {
    return get_frame(s, s->video, min_percent, max_percent);
}

image* source_get_audio_frame(source *s, const double min_percent, const double max_percent) {
    return get_frame(s, s->audio, min_percent, max_percent);
}

int source_has_index(const source *s) {
    return s->has_index;
}

void source_set_exact(source *s, const int exact) {
    s->exact = exact;
    if (s->video) {
        s->video->current_frame = -1;
    }
    if (s->audio) {
        s->audio->current_frame = -1;
    }
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
    av_rdft_end(s->rdft);

    if (s->video) {
        av_free(s->buffer);
        av_frame_free(&s->scaleframe);
        sws_freeContext(s->sws_context);
        stream_free(s->video);
    }

    if (s->audio) {
        stream_free(s->audio);
    }

    avformat_close_input(&s->format);

    if (s->keyframes) {
        free(s->keyframes);
    }

    free(s);
}
