#ifndef INCLUDE_source_h__
#define INCLUDE_source_h__

#include "nordlicht.h"
#include "image.h"

#define COLUMN_PRECISION 0.95 // choose a frame from this percentage at the middle of each column

typedef struct source source;

source* source_init(const char *filename);

int source_has_video(source *s);

int source_has_audio(source *s);

image* source_get_video_frame(source *s, const double min_percent, const double max_percent);

image* source_get_audio_frame(source *s, const double min_percent, const double max_percent);

int source_build_keyframe_index_step(source *s, const int width);

int source_has_index(const source *s);

void source_set_exact(source *s, const int exact);

float source_start(const source *s);

void source_set_start(source *s, const float start);

float source_end(const source *s);

void source_set_end(source *s, const float end);

void source_free(source *s);

#endif
