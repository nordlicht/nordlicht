#include "nordlicht.h"
#include <string.h>
#include <png.h>
#include "error.h"
#include "video.h"

#ifdef _WIN32
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

typedef struct {
    nordlicht_style style;
    int height;
} track;

struct nordlicht {
    int width, height;
    char *filename;
    track *tracks;
    int num_tracks;
    unsigned char *data;
    float start, end;

    int owns_data;
    int modifiable;
    nordlicht_strategy strategy;
    float progress;
    video *source;
};

size_t nordlicht_buffer_size(nordlicht *n) {
    return n->width * n->height * 4;
}

nordlicht* nordlicht_init(char *filename, int width, int height) {
    if (width < 1 || height < 1) {
        error("Dimensions must be positive (got %dx%d)", width, height);
        return NULL;
    }
    nordlicht *n;
    n = malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;
    n->filename = filename;
    n->start = 0.0;
    n->end = 1.0;

    n->data = calloc(nordlicht_buffer_size(n), 1);
    n->owns_data = 1;

    n->num_tracks = 1;
    n->tracks = malloc(sizeof(track));
    n->tracks[0].style = NORDLICHT_STYLE_HORIZONTAL;
    n->tracks[0].height = n->height;

    n->strategy = NORDLICHT_STRATEGY_FAST;
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

char *nordlicht_error() {
    return get_error();
}

int nordlicht_set_start(nordlicht *n, float start) {
    if (! n->modifiable) {
        return -1;
    }

    if (start < 0) {
        error("'start' has to be >= 0.");
        return -1;
    }

    if (start >= n->end) {
        error("'start' has to be smaller than 'end'.");
        return -1;
    }

    n->start = start;
}

int nordlicht_set_end(nordlicht *n, float end) {
    if (! n->modifiable) {
        return -1;
    }

    if (end > 1) {
        error("'end' has to be <= 1.");
        return -1;
    }

    if (n->start >= end) {
        error("'start' has to be smaller than 'end'.");
        return -1;
    }

    n->end = end;
}

int nordlicht_set_style(nordlicht *n, nordlicht_style *styles, int num_tracks) {
    if (! n->modifiable) {
        return -1;
    }

    n->num_tracks = num_tracks;
    free(n->tracks);
    n->tracks = malloc(n->num_tracks*sizeof(track));
    int i;
    for (i=0; i<num_tracks; i++) {
        nordlicht_style s = styles[i];
        if (s < 0 || s > NORDLICHT_STYLE_COLUMN) {
            return -1;
        }

        n->tracks[i].style = s;
        n->tracks[i].height = n->height/n->num_tracks;
    }

    return 0;
}

int nordlicht_set_strategy(nordlicht *n, nordlicht_strategy s) {
    if (! n->modifiable) {
        return -1;
    }
    if (s < 0 || s > NORDLICHT_STRATEGY_LIVE) {
        return -1;
    }
    n->strategy = s;
    return 0;
}

int nordlicht_generate(nordlicht *n) {
    n->modifiable = 0;

    video_build_keyframe_index(n->source, n->width);
    int x, exact;

    int do_a_fast_pass = (n->strategy == NORDLICHT_STRATEGY_LIVE) || !video_exact(n->source);
    int do_an_exact_pass = video_exact(n->source);

    for (exact = (!do_a_fast_pass); exact <= do_an_exact_pass; exact++) {
        video_set_exact(n->source, exact);
        for (x = 0; x < n->width; x++) {
            int i;
            int y_offset = 0;
            for(i = 0; i < n->num_tracks; i++) {
                float proportion = n->end-n->start;
                image *frame = video_get_frame(n->source, 1.0*(x+0.5-COLUMN_PRECISION/2.0)/n->width*proportion + n->start,
                                                          1.0*(x+0.5+COLUMN_PRECISION/2.0)/n->width*proportion + n->start);

                column *c;
                switch (n->tracks[i].style) {
                    case NORDLICHT_STYLE_HORIZONTAL:
                        c = compress_to_column(frame);
                        break;
                    case NORDLICHT_STYLE_VERTICAL:
                        c = compress_to_row(frame);
                        break;
                    case NORDLICHT_STYLE_COLUMN:
                        c = cut_middle_column(frame);
                        break;
                }
                free(frame->data);
                free(frame);

                column *c2 = column_scale(c, n->tracks[i].height);
                free(c->data);
                free(c);

                int y;
                for (y = 0; y < c2->length; y++) {
                    // BGRA pixel format:
                    memcpy(n->data+n->width*4*(y_offset+y)+4*x+2, c2->data+3*y+0, 1);
                    memcpy(n->data+n->width*4*(y_offset+y)+4*x+1, c2->data+3*y+1, 1);
                    memcpy(n->data+n->width*4*(y_offset+y)+4*x+0, c2->data+3*y+2, 1);
                    memset(n->data+n->width*4*(y_offset+y)+4*x+3, 255, 1);
                }
                free(c2->data);
                free(c2);

                y_offset += n->tracks[i].height;
            }

            n->progress = 1.0*x/n->width;
        }
    }

    n->progress = 1.0;
    return 0;
}

int nordlicht_write(nordlicht *n, char *filename) {
    int code = 0;

    if (filename == NULL) {
        error("Output filename must not be NULL");
        return -1;
    }

    if (strcmp(filename, "") == 0) {
        error("Output filename must not be empty");
        return -1;
    }

    char *realpath_output = realpath(filename, NULL);
    if (realpath_output != NULL) {
        // output file exists
        char *realpath_input = realpath(n->filename, NULL);
        if (realpath_input != NULL) {
            // otherwise, input filename is probably a URL

            if (strcmp(realpath_input, realpath_output) == 0) {
                error("Will not overwrite input file");
                code = -1;
            }
            free(realpath_input);
        }
        free(realpath_output);

        if (code != 0) {
            return code;
        }
    }

    FILE *fp;
    png_structp png = NULL;
    png_infop png_info = NULL;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        error("Could not open '%s' for writing", filename);
        code = -1;
        goto finalize;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL) {
        error("Error writing PNG");
        code = -1;
        goto finalize;
    }

    png_info = png_create_info_struct(png);
    if (png_info == NULL) {
        error("Error writing PNG");
        code = -1;
        goto finalize;
    }

    if (setjmp(png_jmpbuf(png))) {
        error("Error writing PNG");
        code = -1;
        goto finalize;
    }
    png_init_io(png, fp);
    png_set_IHDR(png, png_info, n->width, n->height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png, png_info);

    png_set_bgr(png);

    int y;
    for (y = 0; y < n->height; y++) {
        png_write_row(png, n->data+4*y*n->width);
    }

    png_write_end(png, NULL);

finalize:
    if (fp != NULL) fclose(fp);
    if (png_info != NULL) png_free_data(png, png_info, PNG_FREE_ALL, -1);
    if (png != NULL) png_destroy_write_struct(&png, (png_infopp)NULL);

    return code;
}

float nordlicht_progress(nordlicht *n) {
    return n->progress;
}

const unsigned char* nordlicht_buffer(nordlicht *n) {
    return n->data;
}

int nordlicht_set_buffer(nordlicht *n, unsigned char *data) {
    if (! n->modifiable) {
        return -1;
    }

    if (data == NULL) {
        return -1;
    }

    if (n->owns_data) {
        free(n->data);
    }
    n->owns_data = 0;
    n->data = data;
    return 0;
}
