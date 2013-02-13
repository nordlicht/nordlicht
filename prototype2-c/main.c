#include "video-barcode.h"

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

    pthread_t thread = startBarcodeGeneration(barcode, argv[1], width, height);

    while (pthread_kill(thread, 0) != ESRCH) {
        saveFrame(barcode, width, height);
        sleep(1);
    }
    saveFrame(barcode, width, height);

    pthread_join(thread, NULL);

    av_free(barcodeBuffer);
    av_free(barcode);

    return 0;
}
