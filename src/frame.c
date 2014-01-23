#include "common.h"
#include "frame.h"

frame* frame_create(int width, int height, int fill_color) {
    frame *f;
    f = malloc(sizeof(frame));

    f->frame = avcodec_alloc_frame();
    f->frame->width = width;
    f->frame->height = height;
    f->frame->format = PIX_FMT_RGB24;

    f->buffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, width, height));
    avpicture_fill((AVPicture *)f->frame, f->buffer, PIX_FMT_RGB24, width, height);
    memset(f->frame->data[0], fill_color, f->frame->linesize[0]*height);
    return f;
}

void frame_free(frame *f) {
    av_free(f->buffer);
    avcodec_free_frame(&f->frame);
    free(f);
}

void frame_copy(frame *f1, frame *f2, int offset_x, int offset_y) {
    int x, y;
    for (x = 0; x < f1->frame->width; x++) {
        for (y = 0; y < f1->frame->height; y++) {
            int f2_pos = (y+offset_y)*f2->frame->linesize[0]+(x+offset_x)*3;
            int f1_pos = y*f1->frame->linesize[0]+x*3;
            memcpy(f2->frame->data[0]+f2_pos, f1->frame->data[0]+f1_pos, sizeof(uint8_t)*3);
        }
    }
}

frame* frame_scale_unsafe(frame *f, int width, int height) {
    frame *f2 = frame_create(width, height, 0);
    if (width < 8) {
        if (width == 1) {
            if (f->frame->height != height) {
                f = frame_scale(f, f->frame->width, height);
            }
            int x, y;
            for (y = 0; y < height; y++) {
                int r = 0, b = 0, g = 0;
                for (x = 0; x < f->frame->width; x++) {
                    r += f->frame->data[0][y*f->frame->linesize[0]+3*x+0];
                    g += f->frame->data[0][y*f->frame->linesize[0]+3*x+1];
                    b += f->frame->data[0][y*f->frame->linesize[0]+3*x+2];
                }
                f2->frame->data[0][y*f2->frame->linesize[0]+0] = r/f->frame->width;
                f2->frame->data[0][y*f2->frame->linesize[0]+1] = g/f->frame->width;
                f2->frame->data[0][y*f2->frame->linesize[0]+2] = b/f->frame->width;
            }
        } else {
            // TODO
        }
    } else {
        struct SwsContext *sws_context = sws_getContext(f->frame->width, f->frame->height, f->frame->format,
                f2->frame->width, f2->frame->height, f2->frame->format, SWS_AREA, NULL, NULL, NULL);
        sws_scale(sws_context, (uint8_t const * const *)f->frame->data,
                f->frame->linesize, 0, f->frame->height, f2->frame->data,
                f2->frame->linesize);
        sws_freeContext(sws_context);
    }
    return f2;
}

frame* frame_scale(frame *f, int width, int height) {
    frame *tmp;
    while (f->frame->width != width || f->frame->height != height) {
        int w = f->frame->width/(MAX_FILTER_SIZE/2);
        if (w < width) {
            w = width;
        }

        int h = f->frame->height/(MAX_FILTER_SIZE/2);
        if (h < height)
            h = height;

        if (width == 1) {
            w = 1;
        }
        tmp = frame_scale_unsafe(f, w, h);
        f = tmp;
    }
    return f;
}

void frame_write(frame *f, char *file_path) {
    AVCodec *encoder = avcodec_find_encoder_by_name("png");
    AVCodecContext *encoder_context;
    encoder_context = avcodec_alloc_context3(encoder);
    encoder_context->width = f->frame->width;
    encoder_context->height = f->frame->height;
    encoder_context->pix_fmt = PIX_FMT_RGB24;
    if (avcodec_open2(encoder_context, encoder, NULL) < 0) {
        die("Could not open output codec.");
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 28, 0)
    uint8_t buffer[200000]; // TODO: Why this size?
    packet.size = avcodec_encode_video(encoder_context, buffer, 200000, code->frame);
    packet.data = buffer;
#else
    int got_packet = 0;
    avcodec_encode_video2(encoder_context, &packet, f->frame, &got_packet);
    if (! got_packet) {
        die("Encoding error.");
    }
#endif

    FILE *file;
    file = fopen(file_path, "wb");
    if (! file) {
        die("Could not open output file.");
    }
    fwrite(packet.data, 1, packet.size, file);
    fclose(file);

    av_free_packet(&packet);

    avcodec_close(encoder_context);
    av_free(encoder_context);
}
