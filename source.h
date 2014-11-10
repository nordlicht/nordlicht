#ifndef INCLUDE_source_h__
#define INCLUDE_source_h__

#include "nordlicht.h"
#include "image.h"

#define COLUMN_PRECISION 0.95 // choose a frame from this percentage at the middle of each column

typedef struct source source;

source* source_init(const char *filename);

image* source_get_video_frame(source *s, const double min_percent, const double max_percent);

image* source_get_audio_frame(source *s, const double min_percent, const double max_percent);

void source_build_keyframe_index(source *s, const int width);

int source_exact(const source *s);

void source_set_exact(source *s, const int exact);

float source_start(const source *s);

void source_set_start(source *s, const float start);

float source_end(const source *s);

void source_set_end(source *s, const float end);

void source_free(source *s);

#endif
