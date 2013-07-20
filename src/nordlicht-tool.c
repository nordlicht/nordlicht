#include "nordlicht.h"
#include <stdio.h>
#include <unistd.h>

char *gnu_basename(char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

int main(int argc, char** argv) {
    int width = 1024;
    int height = 400;
    char *file_path = NULL;

    if (argc < 2) {
        printf("Usage: nordlicht VIDEOFILE [WIDTH [HEIGHT [BARCODEFILE.png]]]\nDefault size is %dx%d, default BARCODEFILE is \"$(basename VIDEOFILE).png\".\n", width, height);
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

    nordlicht *code;
    nordlicht_create(&code, width, height);

    nordlicht_output(code, output_path);
    nordlicht_input(code, file_path);

    while (nordlicht_progress(code) < 1) {
        printf("\r%02.0f%%", nordlicht_progress(code)*100);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

    nordlicht_free(code);
    return 0;
}
