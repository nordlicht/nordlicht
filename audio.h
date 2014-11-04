#ifndef INCLUDE_audio_h__
#define INCLUDE_audio_h__

#include "nordlicht.h"
#include "graphics.h"

#define COLUMN_PRECISION 0.95 // choose a frame from this percentage at the middle of each column

typedef struct audio audio;

audio* audio_init(const char *filename);

image* audio_get_frame(audio *v, const double min_percent, const double max_percent);

float audio_start(const audio *v);

void audio_set_start(audio *v, const float start);

float audio_end(const audio *v);

void audio_set_end(audio *v, const float end);

void audio_free(audio *v);

#endif
