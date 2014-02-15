#include <pthread.h>
#include <unistd.h>
#include <popt.h>
#include "nordlicht.h"
#include "common.h"

const char *gnu_basename(const char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

void print_help(poptContext popt, int ret) {
    poptPrintHelp(popt, stderr, 0);
    exit(ret);
}

int main(int argc, const char **argv) {
    int width = 1000;
    int height = 150;
    char *output_file = NULL;
    int free_output_file = 0;

    int help = 0;

    struct poptOption optionsTable[] = {
        {"help", '\0', 0, &help, 0, NULL, NULL},
        {"width", 'w', POPT_ARG_INT, &width, 0, "Override default width of 1000 pixels.", NULL},
        {"height", 'h', POPT_ARG_INT, &height, 0, "Override default height of 150 pixels.", NULL},
        {"output", 'o', POPT_ARG_STRING, &output_file, 0, "Set filename of output PNG. Default: $(basename VIDEOFILE).png", "FILENAME"},
        POPT_TABLEEND
    };

    poptContext popt = poptGetContext(NULL, argc, argv, optionsTable, 0);
    poptSetOtherOptionHelp(popt, "VIDEOFILE");

    char c;

    // The next line leaks 3 bytes, blame popt!
    while ((c = poptGetNextOpt(popt)) >= 0) { }

    if (c < -1) {
        fprintf(stderr, "nordlicht: %s: %s\n", poptBadOption(popt, POPT_BADOPTION_NOALIAS), poptStrerror(c));
        return 1;
    }

    if (help) {
        fprintf(stderr, "nordlicht creates colorful barcodes from video files.\n\n");
        print_help(popt, 0);
    }

    char *filename = (char*)poptGetArg(popt);

    if (filename == NULL) {
        error("Please specify an input file.");
        print_help(popt, 1);
    }

    if (poptGetArg(popt) != NULL) {
        error("Please specify only one input file.");
        print_help(popt, 1);
    }

    if (output_file == NULL) {
        output_file = malloc(snprintf(NULL, 0, "%s.png", gnu_basename(filename)) + 1);
        sprintf(output_file, "%s.png", gnu_basename(filename));
        free_output_file = 1;
    }

    // MAIN PART

    nordlicht *code = nordlicht_init(filename, width, height);

    if (code == NULL) {
        return 1;
    }

    // Try to write the empty code to fail early if this does not work
    if (nordlicht_write(code, output_file) != 0) {
        return 1;
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

    if (free_output_file) {
        free(output_file);
    }

    poptFreeContext(popt);
    return 0;
}
