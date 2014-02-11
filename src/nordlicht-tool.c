#include "nordlicht.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

char *gnu_basename(char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

int main(int argc, char** argv) {
    int width = 1000;
    int height = 150;
    char *file_path = NULL;

    if (argc < 2) {
        printf("Usage: nordlicht VIDEOFILE [WIDTH [HEIGHT [OUTPUTFILE.png]]]\nDefault size is %dx%d, default output file is \"$(basename VIDEOFILE).png\".\n", width, height);
        return 1;
    }

    file_path = argv[1];
    char *output_path = malloc(snprintf(NULL, 0, "%s.png", gnu_basename(file_path)) + 1);
    sprintf(output_path, "%s.png", gnu_basename(file_path));

    if (argc >= 3) {
        width = atoi(argv[2]);
    }
    if (argc >= 4) {
        height = atoi(argv[3]);
    }
    if (argc >= 5) {
        free(output_path);
        output_path = argv[4];
    }

    nordlicht *code = nordlicht_init(file_path, width, height);

    if (code == NULL) {
        return -1;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, (void*(*)(void*))nordlicht_generate, code);

    float progress = 0;
    while (progress < 1) {
        progress = nordlicht_progress(code);
        printf("\r%02.0f%%", progress*100);
        usleep(1000);
    }

    nordlicht_write(code, output_path);
    nordlicht_free(code);

    printf(" -> '%s'\n", output_path);
    free(output_path);

    return 0;
}
