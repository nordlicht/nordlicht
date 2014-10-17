#ifndef INCLUDE_video_h__
#define INCLUDE_video_h__

#include "nordlicht.h"
#include "graphics.h"

#define COLUMN_PRECISION 0.95 // choose a frame from this percentage at the middle of each column

typedef struct video video;

video* video_init(char *filename, int width);

column* video_get_column(video *v, double min_percent, double max_percent, nordlicht_style s);

void video_build_keyframe_index(video *v, int width);

int video_exact(video *v);

void video_set_exact(video *v, int exact);

void video_free(video *v);

#endif
