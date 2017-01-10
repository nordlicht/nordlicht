#include "nordlicht.h"
#include <string.h>
#include "error.h"
#include "source.h"

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

    int owns_data;
    int modifiable;
    nordlicht_strategy strategy;
    source *source;

    int current_pass;
    int current_track;
    int current_column;
    int current_y_offset;
    float progress;
};

NORDLICHT_API size_t nordlicht_buffer_size(const nordlicht *n) {
    return n->width * n->height * 4;
}

NORDLICHT_API nordlicht* nordlicht_init(const char *filename, const int width, const int height) {
    if (width < 1 || height < 1) {
        error("Dimensions must be positive (got %dx%d)", width, height);
        return NULL;
    }
    if (width > 100000 || height > 100000) {
        error("Both dimensions may be at most 100000 (got %dx%d)", width, height);
        return NULL;
    }
    nordlicht *n;
    n = (nordlicht *) malloc(sizeof(nordlicht));

    n->width = width;
    n->height = height;

    // prepend "file:" if filename contains a colon and is not a URL,
    // otherwise ffmpeg will fail to open the file
    if (filename && strstr(filename, ":") != 0 && strstr(filename, "://") == 0) {
        size_t filename_len = strlen(filename);
        n->filename = malloc(filename_len + 5 + 1);
        strncpy(n->filename, "file:", 5);
        strncpy(n->filename + 5, filename, filename_len);
        n->filename[filename_len + 5] = '\0';
    } else {
        n->filename = (char *) filename;
    }

    n->data = (unsigned char *) calloc(nordlicht_buffer_size(n), 1);
    if (n->data == 0) {
        error("Not enough memory to allocate %d bytes", nordlicht_buffer_size(n));
        return NULL;
    }

    n->owns_data = 1;

    n->num_tracks = 1;
    n->tracks = (track *) malloc(sizeof(track));
    n->tracks[0].style = NORDLICHT_STYLE_HORIZONTAL;
    n->tracks[0].height = n->height;

    n->strategy = NORDLICHT_STRATEGY_FAST;
    n->modifiable = 1;
    n->source = source_init(n->filename);

    n->current_pass = -1;
    n->current_track = 0;
    n->current_column = 0;
    n->current_y_offset = 0;
    n->progress = 0;

    if (n->source == NULL) {
        error("Could not open video file '%s'", filename);
        free(n);
        return NULL;
    }

    return n;
}

NORDLICHT_API void nordlicht_free(nordlicht *n) {
    if (n->owns_data) {
        free(n->data);
    }
    free(n->tracks);
    source_free(n->source);
    free(n);
}

NORDLICHT_API const char *nordlicht_error() {
    return get_error();
}

NORDLICHT_API int nordlicht_set_start(nordlicht *n, const float start) {
    if (! n->modifiable) {
        return -1;
    }

    if (start < 0) {
        error("'start' has to be greater than or equal to 0.");
        return -1;
    }

    if (start >= source_end(n->source)) {
        error("'start' has to be less than 'end'.");
        return -1;
    }

    source_set_start(n->source, start);
    return 0;
}

NORDLICHT_API int nordlicht_set_end(nordlicht *n, const float end) {
    if (! n->modifiable) {
        return -1;
    }

    if (end > 1) {
        error("'end' has to less than or equal to 1.");
        return -1;
    }

    if (source_start(n->source) >= end) {
        error("'start' has to be less than 'end'.");
        return -1;
    }

    source_set_end(n->source, end);
    return 0;
}

NORDLICHT_API int nordlicht_set_style(nordlicht *n, const nordlicht_style style) {
    if (! n->modifiable) {
        return -1;
    }

    nordlicht_style styles[1] = {style};
    return nordlicht_set_styles(n, styles, 1);
}

NORDLICHT_API int nordlicht_set_styles(nordlicht *n, const nordlicht_style *styles, const int num_tracks) {
    if (! n->modifiable) {
        return -1;
    }

    n->num_tracks = num_tracks;

    if (n->num_tracks > n->height) {
        error("Height of %d px is too low for %d styles", n->height, n->num_tracks);
        return -1;
    }

    free(n->tracks);
    n->tracks = (track *) malloc(n->num_tracks*sizeof(track));

    int height_of_each_track = n->height/n->num_tracks;
    int i;
    for (i=0; i<num_tracks; i++) {
        nordlicht_style s = styles[i];
        if (s > NORDLICHT_STYLE_LAST-1) {
            return -1;
        }

        n->tracks[i].style = s;
        n->tracks[i].height = height_of_each_track;
    }
    n->tracks[0].height = n->height - (n->num_tracks-1)*height_of_each_track;

    return 0;
}

NORDLICHT_API int nordlicht_set_strategy(nordlicht *n, const nordlicht_strategy s) {
    if (! n->modifiable) {
        return -1;
    }
    if (s > NORDLICHT_STRATEGY_LIVE) {
        return -1;
    }
    n->strategy = s;
    return 0;
}

NORDLICHT_API int nordlicht_generate_step(nordlicht *n) {
    n->modifiable = 0;

    if (nordlicht_done(n)) {
        return 0;
    } else if (n->current_pass == -1) {
        // we don't have a (finished) keyframe index yet
        if (source_build_keyframe_index_step(n->source, n->width) == 0) {
            // keyframe index building is done
            if (n->strategy == NORDLICHT_STRATEGY_LIVE || !source_has_index(n->source)) {
                n->current_pass = 0;
            } else {
                n->current_pass = 1;
            }
            source_set_exact(n->source, n->current_pass);
        }
    } else {
        image *frame;

        if (n->tracks[n->current_track].style == NORDLICHT_STYLE_SPECTROGRAM) {
            if (!source_has_audio(n->source)) {
                error("File contains no audio, please select an appropriate style");
                n->progress = 1;
                return -1;
            }
            frame = source_get_audio_frame(n->source, 1.0*(n->current_column+0.5-COLUMN_PRECISION/2.0)/n->width,
                    1.0*(n->current_column+0.5+COLUMN_PRECISION/2.0)/n->width);
        } else {
            if (!source_has_video(n->source)) {
                error("File contains no video, please select an appropriate style");
                n->progress = 1;
                return -1;
            }
            frame = source_get_video_frame(n->source, 1.0*(n->current_column+0.5-COLUMN_PRECISION/2.0)/n->width,
                    1.0*(n->current_column+0.5+COLUMN_PRECISION/2.0)/n->width);
        }

        if (frame != NULL) {
            int thumbnail_width = 1.0*(image_width(frame)*n->tracks[n->current_track].height/image_height(frame));
            image *column = NULL;
            image *tmp = NULL;
            switch (n->tracks[n->current_track].style) {
                case NORDLICHT_STYLE_THUMBNAILS:
                    column = image_scale(frame, thumbnail_width, n->tracks[n->current_track].height);
                    break;
                case NORDLICHT_STYLE_THUMBNAILSTHIRD:
                    tmp = image_cut(frame, 0.333*image_width(frame), 0, 0.333*image_width(frame), image_height(frame));
                    column = image_scale(tmp, 0.333*thumbnail_width, n->tracks[n->current_track].height);
                    image_free(tmp);
                    break;
                case NORDLICHT_STYLE_HORIZONTAL:
                    column = image_scale(frame, 1, n->tracks[n->current_track].height);
                    break;
                case NORDLICHT_STYLE_HORIZONTALTHIRD:
                    tmp = image_cut(frame, 0.333*image_width(frame), 0, 0.333*image_width(frame), image_height(frame));
                    column = image_scale(tmp, 1, n->tracks[n->current_track].height);
                    image_free(tmp);
                    break;
                case NORDLICHT_STYLE_VERTICAL:
                    tmp = image_scale(frame, n->tracks[n->current_track].height, 1);
                    column = image_flip(tmp);
                    image_free(tmp);
                    break;
                case NORDLICHT_STYLE_VERTICALTHIRD:
                    tmp = image_cut(frame, 0, 0.333*image_height(frame), image_width(frame), 0.333*image_height(frame));
                    image *tmp2 = NULL;
                    tmp2 = image_scale(tmp, n->tracks[n->current_track].height, 1);
                    column = image_flip(tmp2);
                    image_free(tmp);
                    image_free(tmp2);
                    break;
                case NORDLICHT_STYLE_SLITSCAN:
                    tmp = image_column(frame, 1.0*(n->current_column%thumbnail_width)/thumbnail_width);
                    column = image_scale(tmp, 1, n->tracks[n->current_track].height);
                    image_free(tmp);
                    break;
                case NORDLICHT_STYLE_SLITSCANTHIRD:
                    tmp = image_column(frame, 0.333+0.333*fmod((3.0*(n->current_column%(thumbnail_width))/thumbnail_width),1));
                    column = image_scale(tmp, 1, n->tracks[n->current_track].height);
                    image_free(tmp);
                    break;
                case NORDLICHT_STYLE_MIDDLECOLUMN:
                    tmp = image_column(frame, 0.5);
                    column = image_scale(tmp, 1, n->tracks[n->current_track].height);
                    image_free(tmp);
                    break;
                case NORDLICHT_STYLE_SPECTROGRAM:
                    column = image_scale(frame, 1, n->tracks[n->current_track].height);
                    break;
                default:
                    // cannot happen (TM)
                    return -1;
                    break;
            }

            image_to_bgra(n->data, n->width, n->height, column, n->current_column, n->current_y_offset);

            n->current_column = n->current_column + image_width(column) - 1;
            if (n->current_column >= n->width) {
                n->current_column = n->width - 1;
            }
            n->progress = (n->current_track+1.0*n->current_column/n->width)/n->num_tracks;

            image_free(column);
        }

        n->current_column++;
        if (n->current_column == n->width) {
            n->current_column = 0;
            n->current_y_offset += n->tracks[n->current_track].height;
            n->current_track++;
            if (n->current_track == n->num_tracks) {
                n->current_track = 0;
                n->current_y_offset = 0;
                n->current_pass++;
                if (n->current_pass == 2 || !source_has_index(n->source)) {
                    // we're done :)
                    n->progress = 1.0;
                    n->current_pass = 2;
                    return 0;
                } else {
                    source_set_exact(n->source, n->current_pass);
                }
            }
        }
    }

    return 0;
}

NORDLICHT_API int nordlicht_generate(nordlicht *n) {
    while(!nordlicht_done(n)) {
        if (nordlicht_generate_step(n) != 0) {
            return -1;
        }
    }
    return 0;
}

NORDLICHT_API int nordlicht_write(const nordlicht *n, const char *filename) {
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

    image *i = image_from_bgra(n->data, n->width, n->height);
    if (image_write_png(i, filename) != 0) {
        return -1;
    }
    image_free(i);

    return code;
}

NORDLICHT_API int nordlicht_done(const nordlicht *n) {
    return n->current_pass == 2;
}

NORDLICHT_API float nordlicht_progress(const nordlicht *n) {
    return n->progress;
}

NORDLICHT_API const unsigned char* nordlicht_buffer(const nordlicht *n) {
    return n->data;
}

NORDLICHT_API int nordlicht_set_buffer(nordlicht *n, unsigned char *data) {
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
