#include "common.h"
#include "nordlicht.h"
#include "frame.h"

#define SLICE_WIDTH 1

struct nordlicht {
    int width, height;
    int frames_written;
    int mutable;

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

frame* get_frame(nordlicht *n, int column) {
    AVFrame *avframe= NULL;
    avframe = avcodec_alloc_frame();

    double fps = 1.0/av_q2d(n->format_context->streams[n->video_stream]->codec->time_base);
    double total_frames = n->format_context->duration/AV_TIME_BASE*fps;
    double frames_step = total_frames/n->width;

    long target_frame = frames_step*(column+0.5);

    av_seek_frame(n->format_context, n->video_stream, target_frame*(1/av_q2d(n->format_context->streams[n->video_stream]->time_base)*av_q2d(n->format_context->streams[n->video_stream]->codec->time_base)), 0);

    AVPacket packet;
    int64_t frameTime = -1;

    int frameFinished = 0;
    int try = 0;
    try++;
    if (try > 100) {
        printf("giving up\n");
        return 0;
    }
    while(av_read_frame(n->format_context, &packet) >= 0) {
        if (packet.stream_index == n->video_stream) {
            avcodec_decode_video2(n->decoder_context, avframe, &frameFinished, &packet);
            frameTime = packet.dts*av_q2d(n->format_context->streams[n->video_stream]->time_base)/av_q2d(n->format_context->streams[n->video_stream]->codec->time_base)/2;
            //printf("%ld/%ld\n", frameTime, target_frame);
            break;
        }
        av_free_packet(&packet);
    }
    av_free_packet(&packet);

    avframe->width = n->decoder_context->width;
    avframe->height = n->decoder_context->height;
    avframe->format = PIX_FMT_YUV420P;
    frame *f = malloc(sizeof(frame));
    f->frame = avframe;
    return frame_scale(f, f->frame->width, f->frame->height);
}

//////////////////////////////////////////////////////////

nordlicht* nordlicht_create(int width, int height) {
    //av_log_set_level(AV_LOG_QUIET);
    av_register_all();

    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->frames_written = 0;
    n->mutable = 1;
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

    int last_frame = n->frames_written + 100;
    if (last_frame > n->width) {
        last_frame = n->width;
    }

    int i;
    frame *s, *f;
    for(i = n->frames_written; i < last_frame; i+=SLICE_WIDTH) {
        f = get_frame(n, i);
        s = frame_scale(f, SLICE_WIDTH, n->height);
        frame_copy(s, n->code, i, 0);
    }

    frame_write(n->code, n->output_file_path);
    n->frames_written = last_frame;
    return 1.0*n->frames_written/n->width;
}
