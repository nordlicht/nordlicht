#include "nordlicht.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

    nordlicht_generate(code);
    nordlicht_write(code, output_path);
    nordlicht_free(code);

    free(output_path);

    return 0;
}
