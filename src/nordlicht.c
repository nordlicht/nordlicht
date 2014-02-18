#include "nordlicht.h"
#include <FreeImage.h>
#include "common.h"
#include "video.h"

struct nordlicht {
    int width, height;
    char *filename;
    unsigned char *data;
    nordlicht_style style;
    int exact;
    int modifiable;
    float progress;
    video *source;
};

nordlicht* nordlicht_init_exact(char *filename, int width, int height, int exact) {
    if (width < 1 || height < 1) {
        error("Dimensions must be positive (got %dx%d)", width, height);
        return NULL;
    }
    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->filename = filename;
    n->data = calloc(width*height*3, 1);
    n->style = NORDLICHT_STYLE_HORIZONTAL;
    n->modifiable = 1;
    n->progress = 0;
    n->source = video_init(filename, exact, width);

    if (n->source == NULL) {
        error("Could not open video file '%s'", filename);
        free(n);
        return NULL;
    }

    return n;
}

nordlicht* nordlicht_init(char *filename, int width, int height) {
    return nordlicht_init_exact(filename, width, height, 0);
}

void nordlicht_free(nordlicht *n) {
    free(n->data);
    video_free(n->source);
    free(n);
}

void nordlicht_set_style(nordlicht *n, nordlicht_style s) {
    if (n->modifiable) {
        n->style = s;
    }
}

unsigned char* get_column(nordlicht *n, int i) {
    column *c = video_get_column(n->source, 1.0*i/n->width, 1.0*(i+1)/n->width, n->style);
    column *c2 = column_scale(c, n->height);
    unsigned char *data = c2->data;
    free(c2);
    column_free(c);
    return data;
}

int nordlicht_generate(nordlicht *n) {
    int x;
    for (x=0; x<n->width; x++) {
        unsigned char *column = get_column(n, x); // TODO: Fill memory directly, no need to memcpy
        memcpy(n->data+n->height*3*x, column, n->height*3);
        free(column);
        n->progress = 1.0*x/n->width;
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

    FIBITMAP *bitmap = FreeImage_ConvertFromRawBits(n->data, n->height, n->width, n->height*3, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, 1);
    FIBITMAP *bitmap2 = FreeImage_Rotate(bitmap, -90, 0);
    FreeImage_FlipHorizontal(bitmap2);
    if (!FreeImage_Save(FreeImage_GetFIFFromMime("image/png"), bitmap2, filename, 0)) {
        error("Could not write to '%s'", filename);
        return -1;
    }
    FreeImage_Unload(bitmap);
    FreeImage_Unload(bitmap2);
    return 0;
}

float nordlicht_progress(nordlicht *n) {
    return n->progress;
}
