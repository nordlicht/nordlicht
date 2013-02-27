#include "mediabarcode.h"
#include "stdio.h"
#include "unistd.h"

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

    mediabarcode *code;
    mbc_create(&code, width, height);

    mbc_output(code, "barcode.ppm");
    mbc_input(code, file_path);

    while (!mbc_is_done(code)) {
        printf("%02.0f%\n", mbc_progress(code)*100);
        sleep(1);
    }

    mbc_free(code);
    return 0;
}
