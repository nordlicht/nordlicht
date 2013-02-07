#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>

#define WIDTH 16
#define HEIGHT 40
#define LENGTH 1024

void SaveFrame(AVFrame *pFrame, int width, int height, int64_t iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "/tmp/frame%06ld.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

    // Close file
    fclose(pFile);
}

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL; 
    AVFrame         *pFrameRGB = NULL;
    AVFrame         *pFrameWide = NULL;
    AVFrame         *pFrameBar = NULL;
    AVPacket        packet;
    int             frameFinished;
    int             numBytes, numBytesWide, numBytesBar;
    uint8_t         *buffer = NULL;
    uint8_t         *bufferWide = NULL;
    uint8_t         *bufferBar = NULL;

    AVDictionary    *optionsDict = NULL;
    struct SwsContext      *sws_ctx = NULL;
    struct SwsContext      *sws_ctx2 = NULL;

    if(argc < 2) {
        printf("Please provide a movie file\n");
        return -1;
    }
    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
        return -1; // Couldn't open file

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    // Find the first video stream
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
        return -1; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return -1; // Could not open codec

    // Allocate video frame
    pFrame=avcodec_alloc_frame();

    // Allocate an AVFrame structure
    pFrameRGB=avcodec_alloc_frame();
    pFrameWide=avcodec_alloc_frame();
    pFrameBar=avcodec_alloc_frame();
    if(pFrameRGB==NULL)
        return -1;

    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
            HEIGHT);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    numBytesWide=avpicture_get_size(PIX_FMT_RGB24, WIDTH*LENGTH,
            HEIGHT);
    bufferWide=(uint8_t *)av_malloc(numBytesWide*sizeof(uint8_t));

    numBytesBar=avpicture_get_size(PIX_FMT_RGB24, LENGTH,
            HEIGHT);
    bufferBar=(uint8_t *)av_malloc(numBytesBar*sizeof(uint8_t));

    sws_ctx =
        sws_getContext
        (
         pCodecCtx->width,
         pCodecCtx->height,
         pCodecCtx->pix_fmt,
         /* pCodecCtx->width, */
         WIDTH,
         HEIGHT,
         PIX_FMT_RGB24,
         SWS_BILINEAR,
         NULL,
         NULL,
         NULL
        );

    sws_ctx2 =
        sws_getContext
        (
         WIDTH*LENGTH,
         HEIGHT,
         PIX_FMT_RGB24,
         LENGTH,
         HEIGHT,
         PIX_FMT_RGB24,
         SWS_BILINEAR,
         NULL,
         NULL,
         NULL
        );

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
            WIDTH, HEIGHT);
    avpicture_fill((AVPicture *)pFrameWide, bufferWide, PIX_FMT_RGB24,
            WIDTH*LENGTH, HEIGHT);
    avpicture_fill((AVPicture *)pFrameBar, bufferBar, PIX_FMT_RGB24,
            LENGTH, HEIGHT);

    int64_t seconds_per_frame = pFormatCtx->duration/LENGTH;

    uint8_t *orig = pFrameWide->data[0];

    int try;
    for(i=0; i<LENGTH; i++) {
        printf("%d\n", i);
        av_seek_frame(pFormatCtx, -1, i*seconds_per_frame, 0);
        frameFinished = 0;
        try=0;
        while(!frameFinished && try<100 ) {
            try++;
        while(av_read_frame(pFormatCtx, &packet)>=0) {
            // Is this a packet from the video stream?
            if(packet.stream_index==videoStream) {
                // Decode video frame
                avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
                        &packet);
                break;
            }
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        }

        // Did we get a video frame?
        if(frameFinished) {
            // Convert the image from its native format to RGB
            sws_scale
                (
                 sws_ctx,
                 (uint8_t const * const *)pFrame->data,
                 pFrame->linesize,
                 0,
                 pCodecCtx->height,
                 pFrameWide->data,
                 pFrameWide->linesize
                );

            pFrameWide->data[0] += WIDTH*3;
        }
    }

    pFrameWide->data[0] = orig;
            sws_scale
                (
                 sws_ctx2,
                 (uint8_t const * const *)pFrameWide->data,
                 pFrameWide->linesize,
                 0,
                 HEIGHT,
                 pFrameBar->data,
                 pFrameBar->linesize
                );

    SaveFrame(pFrameWide, WIDTH*LENGTH, HEIGHT, 1);
    SaveFrame(pFrameBar, LENGTH, HEIGHT, 0);

    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}
