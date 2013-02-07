#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>

void saveFrame(AVFrame *pFrame, int width, int height) {
    FILE *pFile;
    char *szFilename = "barcode.ppm";
    int  y;

    // Open file
    pFile=fopen(szFilename, "wb");
    if (pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for (y=0; y<height; y++) {
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    }

    // Close file
    fclose(pFile);
}

int decodeFrame(AVFrame *frame, AVFormatContext *formatContext, AVCodecContext *codecContext, int videoStream, long time) {
    av_seek_frame(formatContext, -1, time , 0);
    int frameFinished = 0;
    int try = 0;
    AVPacket packet;
    while(!frameFinished) {
        try++;
        if (try > 100) {
            printf("oops\n");
            return 0;
        }
        while(av_read_frame(formatContext, &packet) >= 0) {
            // Is this a packet from the video stream?
            if (packet.stream_index == videoStream) {
                // Decode video frame
                avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);
                break;
            }
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    return 1;
}

int createBarcode(AVFrame *barcode, char *filename, int width, int height) {
    int frameWidth = 16;
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext  *pCodecCtx = NULL;
    int             i, videoStream;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL; 
    AVFrame         *pFrameWide = NULL;
    int             numBytes, numBytesWide;
    uint8_t         *buffer = NULL;
    uint8_t         *bufferWide = NULL;

    AVDictionary    *optionsDict = NULL;
    struct SwsContext      *sws_ctx = NULL;
    struct SwsContext      *sws_ctx2 = NULL;

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
        return 0; // Couldn't open file

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return 0; // Couldn't find stream information

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, filename, 0);

    // Find the first video stream
    videoStream = -1;
    for (i=0; i<pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    if (videoStream == -1)
        return 0; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return 0; // Codec not found
    }
    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return 0; // Could not open codec

    // Allocate video frame
    pFrame = avcodec_alloc_frame();

    // Allocate an AVFrame structure
    pFrameWide = avcodec_alloc_frame();

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, height);
    buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    numBytesWide =avpicture_get_size(PIX_FMT_RGB24, frameWidth*width, height);
    bufferWide = (uint8_t *)av_malloc(numBytesWide*sizeof(uint8_t));

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                             frameWidth, height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    sws_ctx2 = sws_getContext(frameWidth*width, height, PIX_FMT_RGB24,
                              width, height, PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

    // Assign appropriate parts of buffer to image planes in pFrameWide
    // Note that pFrameWide is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameWide, bufferWide, PIX_FMT_RGB24, frameWidth*width, height);

    int64_t seconds_per_frame = pFormatCtx->duration/width;

    uint8_t *orig = pFrameWide->data[0];

    int try;
    for (i = 0; i<width; i++) {
        printf("%d\n", i);

        if (decodeFrame(pFrame, pFormatCtx, pCodecCtx, videoStream, i*seconds_per_frame)) {
            // Convert the image from its native format to RGB
            sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                    pFrameWide->data, pFrameWide->linesize);
            pFrameWide->data[0] += frameWidth*3;
        }
    }

    pFrameWide->data[0] = orig;
    sws_scale(sws_ctx2, (uint8_t const * const *)pFrameWide->data, pFrameWide->linesize, 0, height,
              barcode->data, barcode->linesize);

    av_free(buffer);
    av_free(bufferWide);
    av_free(pFrame);
    av_free(pFrameWide);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 1;
}

int main(int argc, char *argv[]) {
    int width = 1024;
    int height = 40;

    if (argc < 2) {
        printf("Please provide a movie file\n");
        return -1;
    }
    if (argc >= 3) {
        width = atoi(argv[2]);
    }
    if (argc >= 4) {
        height = atoi(argv[3]);
    }

    AVFrame *barcode;
    uint8_t *barcodeBuffer = NULL;

    barcode = avcodec_alloc_frame();
    barcodeBuffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, width, height));
    avpicture_fill((AVPicture *)barcode, barcodeBuffer, PIX_FMT_RGB24, width, height);

    if (createBarcode(barcode, argv[1], width, height)) {
        saveFrame(barcode, width, height);
    } else {
        printf("Barcode couln't be generated.\n");
    }

    av_free(barcodeBuffer);
    av_free(barcode);

    return 0;
}
