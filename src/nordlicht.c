#include "nordlicht.h"
#include <FreeImage.h>
#include "common.h"
#include "video.h"

struct nordlicht {
    int width, height;
    char *filename;
    unsigned char *data;
    int live;
    nordlicht_style style;
    int modifiable;
    int owns_data;
    float progress;
    video *source;
};

size_t nordlicht_buffer_size(nordlicht *n) {
    return n->width*n->height*4;
}

nordlicht* nordlicht_init(char *filename, int width, int height, int live) {
    if (width < 1 || height < 1) {
        error("Dimensions must be positive (got %dx%d)", width, height);
        return NULL;
    }
    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->filename = filename;

    n->data = calloc(nordlicht_buffer_size(n), 1);
    n->owns_data = 1;

    n->live = !!live;

    n->style = NORDLICHT_STYLE_HORIZONTAL;
    n->modifiable = 1;
    n->progress = 0;
    n->source = video_init(filename, width);

    if (n->source == NULL) {
        error("Could not open video file '%s'", filename);
        free(n);
        return NULL;
    }

    return n;
}

void nordlicht_free(nordlicht *n) {
    if (n->owns_data) {
        free(n->data);
    }
    video_free(n->source);
    free(n);
}

void nordlicht_set_style(nordlicht *n, nordlicht_style s) {
    if (n->modifiable) {
        n->style = s;
    }
}

unsigned char* get_column(nordlicht *n, int i) {
    column *c = video_get_column(n->source, 1.0*(i+0.5-COLUMN_PRECISION/2.0)/n->width, 1.0*(i+0.5+COLUMN_PRECISION/2.0)/n->width, n->style);

    if (c == NULL) {
        return NULL;
    }

    column *c2 = column_scale(c, n->height);
    unsigned char *data = c2->data;
    free(c2);
    return data;
}

int nordlicht_generate(nordlicht *n) {
    video_build_keyframe_index(n->source, n->width);
    int x, exact;

    int do_a_fast_pass = n->live || !video_exact(n->source);
    int do_an_exact_pass = video_exact(n->source);

    for(exact=(!do_a_fast_pass); exact<=do_an_exact_pass; exact++) {
        video_set_exact(n->source, exact);
        for (x=0; x<n->width; x++) {
            unsigned char *column = get_column(n, x); // TODO: Fill memory directly, no need to memcpy
            if (column) {
                int y;
                for (y=0; y<n->height; y++) {
                    memcpy(n->data+n->width*4*y+4*x, column+3*y, 3);
                    memset(n->data+n->width*4*y+4*x+3, 255, 1);
                }
                free(column);
            } else {
                memset(n->data+n->height*3*x, 0, n->height*3);
            }
            n->progress = 1.0*x/n->width;
        }
    }

    n->progress = 1.0;
    return 0;
}

int nordlicht_write(nordlicht *n, char *filename) {
    if (strcmp(filename, "") == 0) {
        error("Output filename must not be empty");
        return -1;
    }

    char *realpath_output = realpath(filename, NULL);
    if (realpath_output != NULL) {
        // output file exists
        char *realpath_input = realpath(n->filename, NULL);
        if (strcmp(realpath_input, realpath_output) == 0) {
            error("Will not overwrite input file");
            return -1;
        }
        free(realpath_input);
        free(realpath_output);
    }

    FIBITMAP *bitmap = FreeImage_ConvertFromRawBits(n->data, n->width, n->height, n->width*4, 4*8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, 1);
    if (!FreeImage_Save(FreeImage_GetFIFFromMime("image/png"), bitmap, filename, 0)) {
        error("Could not write to '%s'", filename);
        return -1;
    }
    FreeImage_Unload(bitmap);
    return 0;
}

float nordlicht_progress(nordlicht *n) {
    return n->progress;
}

const char* nordlicht_buffer(nordlicht *n) {
    return n->data;
}

int nordlicht_set_buffer(nordlicht *n, char *data) {
    n->owns_data = 0;
    free(n->data);
    n->data = data;
    return 0;
}

