#include <pthread.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <popt.h>
#include "nordlicht.h"
#include "version.h"

typedef struct {
    const char *name;
    const char *description;
    nordlicht_style style;
} style;

const style style_table[] = {
    {"horizontal", "compress frames to vertical lines and append them", NORDLICHT_STYLE_HORIZONTAL},
    {"vertical", "compress frames to horizontal lines and rotate them counterclockwise by 90 degrees", NORDLICHT_STYLE_VERTICAL},
    {"slitscan", "take single columns while constantly moving to the right (and wrapping back to the left)", NORDLICHT_STYLE_SLITSCAN},
    {"middlecolumn", "take the middlemost column of each frame", NORDLICHT_STYLE_MIDDLECOLUMN},
    {"thumbnails", "display small thumbnails at regular intervals", NORDLICHT_STYLE_THUMBNAILS},
    {NULL, NULL, NORDLICHT_STYLE_LAST}
};

void print_error(const char *message, ...) {
    fprintf(stderr, "nordlicht: ");
    va_list arglist;
    va_start(arglist, message);
    vfprintf(stderr, message, arglist);
    va_end(arglist);
    fprintf(stderr, "\n");
}

const char *gnu_basename(const char *path) {
    const char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

const char *filename_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return "";
    return dot+1;
}

void print_help(const poptContext popt, const int ret) {
    poptPrintHelp(popt, ret == 0 ? stdout : stderr, 0);

    printf("\nStyles:\n");
    int i;
    for (i = 0; style_table[i].name; i++) {
        printf("  %-14s %s\n", style_table[i].name, style_table[i].description);
    }

    printf("\n\
Examples:\n\
  nordlicht video.mp4                                   generate video.mp4.png of 1000 x 100 pixels size\n\
  nordlicht video.mp4 --style=vertical                  compress individual frames to columns\n\
  nordlicht video.mp4 -w 1920 -h 200 -o barcode.png     override size and name of the output file\n");

    exit(ret);
}

int main(const int argc, const char **argv) {
    int width = -1;
    int height = -1;
    float start = 0.0;
    float end = 1.0;
    char *output_file = NULL;
    char *styles_string = NULL;
    nordlicht_style style;
    nordlicht_strategy strategy;
    int free_output_file = 0;

    int quiet = 0;
    int help = 0;
    int version = 0;

    const struct poptOption optionsTable[] = {
        {"width", 'w', POPT_ARG_INT, &width, 0, "set the barcode's width; by default it's \"height*10\", or 1000 pixels, if both are undefined", NULL},
        {"height", 'h', POPT_ARG_INT, &height, 0, "set the barcode's height; by default it's \"width/10\"", NULL},
        {"output", 'o', POPT_ARG_STRING, &output_file, 0, "set output filename, the default is $(basename VIDEOFILE).png; when you specify an *.bgra file, you'll get a raw 32-bit BGRA file that is updated as the barcode is generated", "FILENAME"},
        {"style", 's', POPT_ARG_STRING, &styles_string, 0, "default is 'horizontal', see \"Styles\" section below. You can specify more than one style, separated by '+', to get multiple tracks", "STYLE"},
        {"start", '\0', POPT_ARG_FLOAT, &start, 0, "specify where to start the barcode (in percent between 0 and 1)", NULL},
        {"end", '\0', POPT_ARG_FLOAT, &end, 0, "specify where to end the barcode (in percent between 0 and 1)", NULL},
        {"quiet", 'q', 0, &quiet, 0, "don't show progress indicator", NULL},
        {"help", '\0', 0, &help, 0, "display this help and exit", NULL},
        {"version", '\0', 0, &version, 0, "output version information and exit", NULL},
        POPT_TABLEEND
    };

    const poptContext popt = poptGetContext(NULL, argc, argv, optionsTable, 0);
    poptSetOtherOptionHelp(popt, "[OPTION]... VIDEOFILE\n\nOptions:");

    char c;

    // The next line leaks 3 bytes, blame popt!
    while ((c = poptGetNextOpt(popt)) >= 0) { }

    if (c < -1) {
        fprintf(stderr, "nordlicht: %s: %s\n", poptBadOption(popt, POPT_BADOPTION_NOALIAS), poptStrerror(c));
        return 1;
    }

    if (version) {
      printf("nordlicht %s\n\nWritten by Sebastian Morr and contributors.\n", NORDLICHT_VERSION);
      return 0;
    }

    if (help) {
        print_help(popt, 0);
    }

    const char *filename = (char*)poptGetArg(popt);

    if (filename == NULL) {
        print_error("Please specify an input file.\n");
        print_help(popt, 1);
    }

    if (poptGetArg(popt) != NULL) {
        print_error("Please specify only one input file.\n");
        print_help(popt, 1);
    }

    if (output_file == NULL) {
        output_file = (char *) malloc(snprintf(NULL, 0, "%s.png", gnu_basename(filename)) + 1);
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

    if (styles_string == NULL) {
        styles_string = "horizontal";
    }

    // count the occurrences of "+" in the styles_string
    const char *s = styles_string;
    int num_tracks;
    for (num_tracks=0; s[num_tracks]; s[num_tracks]=='+' ? num_tracks++ : *s++);
    num_tracks++;

    nordlicht_style *styles;
    styles = (nordlicht_style *) malloc(num_tracks * sizeof(nordlicht_style));

    const char *style_string;
    num_tracks = 0;
    while ((style_string = strsep(&styles_string, "+"))) {
        int i;
        for (i = 0; style_table[i].name; i++) {
            if (strcmp(style_string, style_table[i].name) == 0) {
                styles[num_tracks] = style_table[i].style;
                break;
            }
        }

        if (!style_table[i].name) {
            print_error("Unknown style '%s'.\n", style_string);
            print_help(popt, 1);
        }
        num_tracks++;
    }

    const char *ext = filename_ext(output_file);
    if (strcmp(ext, "png") == 0) {
        strategy = NORDLICHT_STRATEGY_FAST;
    } else if (strcmp(ext, "bgra") == 0) {
        strategy = NORDLICHT_STRATEGY_LIVE;
    } else {
        print_error("Unsupported file extension '%s'.\n", ext);
        print_help(popt, 1);
    }

    // Interesting stuff begins here!

    nordlicht *n = nordlicht_init(filename, width, height);
    unsigned char *data = NULL;

    if (n == NULL) {
        print_error(nordlicht_error());
        exit(1);
    }

    nordlicht_set_start(n, start);
    nordlicht_set_end(n, end);
    nordlicht_set_style(n, styles, num_tracks);
    nordlicht_set_strategy(n, strategy);

    if (nordlicht_error() != NULL) {
        print_error(nordlicht_error());
        exit(1);
    }

    if (strategy == NORDLICHT_STRATEGY_LIVE) {
        int fd = open(output_file, O_CREAT | O_TRUNC | O_RDWR, 0666);
        if (fd == -1) {
            print_error("Could not open '%s'.", output_file);
            exit(1);
        }
        ftruncate(fd, nordlicht_buffer_size(n));
        data = (unsigned char *) mmap(NULL, nordlicht_buffer_size(n), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        nordlicht_set_buffer(n, data);
        close(fd);
    } else {
        // Try to write the empty buffer to fail early if this does not work
        if (nordlicht_write(n, output_file) != 0) {
            print_error(nordlicht_error());
            exit(1);
        }
    }

    pthread_t thread;
    pthread_create(&thread, NULL, (void*(*)(void*))nordlicht_generate, n);

    float progress = 0;

    if (! quiet) {
        printf("nordlicht: Building keyframe index... ");
        fflush(stdout);
        while (progress == 0) {
            progress = nordlicht_progress(n);
            usleep(100000);
        }
        printf("done.\n");

        while (progress < 1) {
            progress = nordlicht_progress(n);
            printf("\rnordlicht: %02.0f%%", progress*100);
            fflush(stdout);
            usleep(100000);
        }
    }

    pthread_join(thread, NULL);

    if (strategy != NORDLICHT_STRATEGY_LIVE) {
        if (nordlicht_write(n, output_file) != 0) {
            print_error(nordlicht_error());
            exit(1);
        }
    }

    free(styles);
    munmap(data, nordlicht_buffer_size(n));
    nordlicht_free(n);

    if (! quiet) {
        printf(" -> '%s'\n", output_file);
    }

    if (free_output_file) {
        free(output_file);
    }

    poptFreeContext(popt);
    return 0;
}
