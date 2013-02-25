#include "vidcode.h"
#include "stdio.h"
#include "unistd.h"

void save_frame(AVFrame *pFrame, int width, int height) {
    FILE *pFile;
    char *szFilename = "vidcode.ppm";
    int  y;

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

int main(int argc, char** argv) {
    int width = 1024;
    int height = 400;
    char *file_path = NULL;

    if (argc < 2) {
        printf("Please provide a video file\n");
        return 1;
    }
    if (argc >= 3) {
        width = atoi(argv[2]);
    }
    if (argc >= 4) {
        height = atoi(argv[3]);
    }
    file_path = argv[1];

    vidcode *code;
    vidcode_create(&code, width, height);

    vidcode_input(code, file_path);

    while (!vidcode_is_done(code)) {
        printf("%02.0f%\n", vidcode_progress(code)*100);
        sleep(1);
    }
    save_frame(code->frame, width, height);

    vidcode_free(code);
    return 0;
}
