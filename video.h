#ifndef INCLUDE_video_h__
#define INCLUDE_video_h__

#include "nordlicht.h"
#include "graphics.h"

#define COLUMN_PRECISION 0.95 // choose a frame from this percentage at the middle of each column

typedef struct video video;

video* video_init(const char *filename);

image* video_get_frame(video *v, const double min_percent, const double max_percent);

void video_build_keyframe_index(video *v, const int width);

int video_exact(const video *v);

void video_set_exact(video *v, const int exact);

float video_start(const video *v);

void video_set_start(video *v, const float start);

float video_end(const video *v);

void video_set_end(video *v, const float end);

void video_free(video *v);

#endif
