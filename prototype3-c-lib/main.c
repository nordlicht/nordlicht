#include "vidcode.h"
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    int width = 1024;
    int height = 400;
    char *file_path = NULL;

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
    file_path = argv[1];

    vidcode *code;
    vidcode_create(&code, width, height);

    vidcode_input(code, file_path);
    while (!vidcode_is_done(code)) {
        printf("%02.0f%\n", code->progress*100);
        sleep(1);
    }

    vidcode_free(code);
}
