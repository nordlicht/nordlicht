#include "common.h"
#include "nordlicht.h"
#include "frame.h"

#define SLICE_WIDTH 1

struct nordlicht {
    int width, height;
    int frames_written;
    int mutable;
    int exact;

    char *input_file_path;
    char *output_file_path;

    frame *code;

    AVFormatContext *format_context;
    AVCodecContext *decoder_context;
    int video_stream;
};

void assert_mutable(nordlicht *n) {
    if (! n->mutable) {
        die("After the first step, input and output are immutable.");
        exit(1);
    }
}

double fps(nordlicht *n) {
    return av_q2d(n->format_context->streams[n->video_stream]->r_frame_rate);
}

double duration_sec(nordlicht *n) {
    return (double)n->format_context->duration / (double)AV_TIME_BASE;
}

int total_number_of_frames(nordlicht *n) {
    return fps(n)*duration_sec(n);
}

frame* grab_frame(nordlicht *n, int scale) {
    int valid = 0;
    int got_frame = 0;

    AVFrame *avframe = NULL;
    avframe = avcodec_alloc_frame();
    AVPacket packet;

    double pts;

    while (!valid) {
        av_read_frame(n->format_context, &packet);
        avcodec_decode_video2(n->decoder_context, avframe, &got_frame, &packet);
        if (got_frame) {
            pts = packet.pts;
            // to sec:
            pts = pts*(double)av_q2d(n->format_context->streams[n->video_stream]->time_base);
            // to frame number:
            pts = pts*fps(n);
            valid = 1;
        }
        av_free_packet(&packet);
    }

    frame *f = malloc(sizeof(frame));
    f->frame = avframe;
    f->pts = pts;
    if (scale) {
        return frame_scale(f, f->frame->width, f->frame->height);
    } else {
        return f;
    }
}

int seek(nordlicht *n, long frame_nr) {
    double sec = frame_nr/fps(n);
    double time_stamp = n->format_context->streams[n->video_stream]->start_time;
    double time_base = av_q2d(n->format_context->streams[n->video_stream]->time_base);
    time_stamp += sec/time_base + 0.5;
    av_seek_frame(n->format_context, n->video_stream, time_stamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(n->format_context->streams[n->video_stream]->codec);

    long grabbed_frame_nr = -1;
    frame *theframe;
    while (grabbed_frame_nr < frame_nr) {
        theframe = grab_frame(n, 0);
        grabbed_frame_nr = theframe->pts;
        //return 0;
    }
}

frame* get_frame(nordlicht *n, long frame) {
    seek(n, frame);
    return grab_frame(n, 1);
}

nordlicht* nordlicht_create(int width, int height) {
    av_log_set_level(AV_LOG_QUIET);
    av_register_all();

    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->frames_written = 0;
    n->mutable = 1;
    n->exact = 0;
    n->code = frame_create(width, height, 0);

    n->format_context = NULL;
    // TODO: null things

    return n;
}

int nordlicht_free(nordlicht *n) {
    frame_free(n->code);
    free(n);
}

int nordlicht_set_input(nordlicht *n, char *file_path) {
    assert_mutable(n);
    n->input_file_path = file_path;

    if (avformat_open_input(&n->format_context, n->input_file_path, NULL, NULL) != 0)
        return 0;
    if (avformat_find_stream_info(n->format_context, NULL) < 0)
        return 0;
    n->video_stream = av_find_best_stream(n->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (n->video_stream == -1)
        return 0;
    n->decoder_context = n->format_context->streams[n->video_stream]->codec;
    AVCodec *codec = NULL;
    codec = avcodec_find_decoder(n->decoder_context->codec_id);
    if (codec == NULL) {
        die("Unsupported codec!");
        return 0;
    }
    AVDictionary *optionsDict = NULL;
    if (avcodec_open2(n->decoder_context, codec, &optionsDict)<0)
        return 0;
}

int nordlicht_set_output(nordlicht *n, char *file_path) {
    assert_mutable(n);
    n->output_file_path = file_path;
}

float nordlicht_step(nordlicht *n) {
    n->mutable = 0;

    int fr = total_number_of_frames(n);

    int last_frame = n->frames_written + 100;
    if (last_frame > n->width) {
        last_frame = n->width;
    }

    int i;
    frame *s, *f;
    for(i = n->frames_written; i < last_frame; i+=SLICE_WIDTH) {
        f = get_frame(n, i*fr/n->width);
        if (f == NULL) {
            continue;
        }
        s = frame_scale(f, SLICE_WIDTH, n->height);
        frame_copy(s, n->code, i, 0);
    }

    frame_write(n->code, n->output_file_path);
    n->frames_written = last_frame;
    return (1.0*n->frames_written/n->width);
}
