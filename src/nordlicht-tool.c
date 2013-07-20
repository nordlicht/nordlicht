#include "nordlicht.h"
#include <stdio.h>
#include <unistd.h>

char *gnu_basename(char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

char *remove_ext(char *filename) {
    char *retstr;
    if ((retstr = malloc (strlen (filename) + 1)) == NULL)
        return NULL;
    strcpy (retstr, filename);
    char *lastdot = strrchr (retstr, '.');
    if (lastdot != NULL)
        *lastdot = '\0';
    return retstr;
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

    nordlicht *code;
    nordlicht_create(&code, width, height);

    char *basename = gnu_basename(file_path);
    char *output_path = remove_ext(basename);
    strcat(output_path, ".png");
    nordlicht_output(code, output_path);

    nordlicht_input(code, file_path);

    while (!nordlicht_is_done(code)) {
        printf("\r%02.0f%%", nordlicht_progress(code)*100);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");

    nordlicht_free(code);
    return 0;
}
