#include "nordlicht.h"
#include "common.h"
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
    char *filename = NULL;

    if (argc < 2 || argc > 5) {
        printf("Usage: nordlicht VIDEOFILE [WIDTH [HEIGHT [OUTPUTFILE.png]]]\nDefault size is %dx%d, default output file is \"$(basename VIDEOFILE).png\".\n", width, height);
        return 1;
    }

    filename = argv[1];
    char *output_file = malloc(snprintf(NULL, 0, "%s.png", gnu_basename(filename)) + 1);
    sprintf(output_file, "%s.png", gnu_basename(filename));

    if (argc >= 3) {
        width = atoi(argv[2]);
    }
    if (argc >= 4) {
        height = atoi(argv[3]);
    }
    if (argc >= 5) {
        free(output_file);
        output_file = argv[4];
    }

    if (strcmp(output_file, "") == 0) {
        error("Output filename must not be empty");
        return -1;
    }

    nordlicht *code = nordlicht_init(filename, width, height);

    if (code == NULL) {
        return -1;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, (void*(*)(void*))nordlicht_generate, code);

    float progress = 0;
    while (progress < 1) {
        progress = nordlicht_progress(code);
        printf("\r");
        printf("[0;34m"); printf("n");
        printf("[1;32m"); printf("o");
        printf("[0;33m"); printf("r");
        printf("[1;31m"); printf("d");
        printf("[0;35m"); printf("l");
        printf("[1;32m"); printf("i");
        printf("[0;31m"); printf("c");
        printf("[1;34m"); printf("h");
        printf("[0;33m"); printf("t");
        printf("[0m");
        printf(": %02.0f%%", progress*100);
        fflush(stdout);
        usleep(100000);
    }
    pthread_join(thread, NULL);

    nordlicht_write(code, output_file);
    nordlicht_free(code);
    printf(" -> '%s'\n", output_file);

    if (argc < 5) {
        free(output_file);
    }

    return 0;
}
