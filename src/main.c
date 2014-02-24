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
    poptPrintHelp(popt, ret == 0 ? stdout : stderr, 0);
    exit(ret);
}

void interesting_stuff(char *filename, char *output_file, int width, int height, nordlicht_style style) {
    nordlicht *code = nordlicht_init(filename, width, height);

    if (code == NULL) {
        exit(1);
    }

    nordlicht_set_style(code, style);

    // Try to write the empty code to fail early if this does not work
    if (nordlicht_write(code, output_file) != 0) {
        exit(1);
    }

    pthread_t thread;
    pthread_create(&thread, NULL, (void*(*)(void*))nordlicht_generate, code);

    float progress = 0;
    while (progress < 1) {
        progress = nordlicht_progress(code);
        printf("\rnordlicht: %02.0f%%", progress*100);
        fflush(stdout);
        usleep(100000);
    }
    pthread_join(thread, NULL);

    nordlicht_write(code, output_file);
    nordlicht_free(code);
    printf(" -> '%s'\n", output_file);
}

int main(int argc, const char **argv) {
    int width = -1;
    int height = -1;
    char *output_file = NULL;
    char *style_string = NULL;
    nordlicht_style style;
    int free_output_file = 0;

    int help = 0;
    int version = 0;

    struct poptOption optionsTable[] = {
        {"help", '\0', 0, &help, 0, NULL, NULL},
        {"version", '\0', 0, &version, 0, NULL, NULL},
        {"width", 'w', POPT_ARG_INT, &width, 0, "Override default width of 1000 pixels.", NULL},
        {"height", 'h', POPT_ARG_INT, &height, 0, "Override default height of 150 pixels.", NULL},
        {"output", 'o', POPT_ARG_STRING, &output_file, 0, "Set filename of output PNG. Default: $(basename VIDEOFILE).png", "FILENAME"},
        {"style", 's', POPT_ARG_STRING, &style_string, 0, "Default is 'horizontal'. Can also be 'vertical', which compresses the frames \"down\" to rows, rotates them counterclockwise by 90 degrees and then appends them.", "STYLE"},
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

    if (version) {
      printf("nordlicht %s\n", NORDLICHT_VERSION);
      return 0;
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

    if (width == -1 && height != -1) {
        width = height*10;
    }
    if (height == -1 && width != -1) {
        height = width/10;
        if (height < 1) {
            height = 1;
        }
    }
    if (height == -1 && width == -1) {
        width = 1000;
        height = 100;
    }

    if (style_string == NULL) {
        style = NORDLICHT_STYLE_HORIZONTAL;
    } else {
        if (strcmp(style_string, "horizontal") == 0) {
            style = NORDLICHT_STYLE_HORIZONTAL;
        } else if (strcmp(style_string, "vertical") == 0) {
            style = NORDLICHT_STYLE_VERTICAL;
        } else {
            error("Unknown style '%s'.", style_string);
            print_help(popt, 1);
        }
    }

    interesting_stuff(filename, output_file, width, height, style);

    if (free_output_file) {
        free(output_file);
    }

    poptFreeContext(popt);
    return 0;
}
