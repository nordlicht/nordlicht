#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__

typedef struct nordlicht nordlicht;

typedef enum nordlicht_style {
    NORDLICHT_STYLE_HORIZONTAL, // compress frames to columns, "move to the right"
    NORDLICHT_STYLE_VERTICAL, // compress frames to rows, "move downwards"
} nordlicht_style;

// Allocate a new nordlicht of specific width and height, for a given video
// file. Use `nordlicht_free` to free it again. Returns NULL on errors.
nordlicht* nordlicht_init(char *filename, int width, int height);

// Experimental: If you set `exact` to 1, nordlicht will produce an "exact"
// barcode. You should activate this if your video has few keyframes. This will
// soon happen automatically.
nordlicht* nordlicht_init_exact(char *filename, int width, int height, int exact);

// Free a nordlicht.
void nordlicht_free(nordlicht *n);

// Set the output style of the nordlicht. Default is NORDLICHT_STYLE_HORIZONTAL.
void nordlicht_set_style(nordlicht *n, nordlicht_style s);

// Generate the nordlicht. Calling this will freeze the nordlicht:
// "set" functions will be without effect. Returns 0 on success.
int nordlicht_generate(nordlicht *n);

// Returns a value between 0 and 1 indicating how much of the nordlicht is done.
float nordlicht_progress(nordlicht *n);

// Write the nordlicht to a PNG file. Return 0 on success.
int nordlicht_write(nordlicht *n, char *filename);

#endif
