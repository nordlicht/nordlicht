#include "video-barcode.h"

int isDone = 0;
AVFrame *barcode;
int width = 1024;
int height = 400;

void done() {
    printf("done\n");
    isDone = 1;
}

void saveFrame() {
    printf("draw\n");
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
        fwrite(barcode->data[0]+y*barcode->linesize[0], 1, width*3, pFile);
    }

    // Close file
    fclose(pFile);
}


int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Please provide a video file\n");
        return -1;
    }
    if (argc >= 3) {
        width = atoi(argv[2]);
    }
    if (argc >= 4) {
        height = atoi(argv[3]);
    }

    uint8_t *barcodeBuffer = NULL;
    barcode = avcodec_alloc_frame();
    barcodeBuffer = (uint8_t *)av_malloc(sizeof(uint8_t)*avpicture_get_size(PIX_FMT_RGB24, width, height));
    avpicture_fill((AVPicture *)barcode, barcodeBuffer, PIX_FMT_RGB24, width, height);

    pthread_t thread = startBarcodeGeneration(barcode, argv[1], width, height, done, saveFrame);

    while (!isDone) {
        sleep(1);
    }

    av_free(barcodeBuffer);
    av_free(barcode);

    return 0;
}
