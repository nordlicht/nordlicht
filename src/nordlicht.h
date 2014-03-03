#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__
#include <stdlib.h> // for size_t

typedef struct nordlicht nordlicht;

typedef enum nordlicht_style {
    NORDLICHT_STYLE_HORIZONTAL, // compress frames to columns, "move to the right"
    NORDLICHT_STYLE_VERTICAL, // compress frames to rows, "move downwards"
} nordlicht_style;

// Allocate a new nordlicht of specific width and height, for a given video
// file. When `live` is true, give a fast approximation before starting the
// slow, exact generation. Use `nordlicht_free` to free the nordlicht again.
// Returns NULL on errors.
nordlicht* nordlicht_init(char *filename, int width, int height, int live);

// Free a nordlicht.
void nordlicht_free(nordlicht *n);

// Set the output style of the nordlicht. Default is NORDLICHT_STYLE_HORIZONTAL.
void nordlicht_set_style(nordlicht *n, nordlicht_style s);

// Returns a pointer to the nordlicht's internal buffer. You can use it to draw
// the barcode while it is generated.
const unsigned char* nordlicht_buffer(nordlicht *n);

// Replace the internal nordlicht's internal buffer. The data pointer is owned
// by the caller and must be freed after `nordlicht_free`. Returns 0 on success.
int nordlicht_set_buffer(nordlicht *n, unsigned char *data);

// Returns the size of this nordlicht's buffer in bytes.
size_t nordlicht_buffer_size(nordlicht *n);

// Generate the nordlicht. Calling this will freeze the nordlicht:
// "set" functions will be without effect. Returns 0 on success.
int nordlicht_generate(nordlicht *n);

// Returns a value between 0 and 1 indicating how much of the nordlicht is done.
float nordlicht_progress(nordlicht *n);

// Write the nordlicht to a PNG file. Returns 0 on success.
int nordlicht_write(nordlicht *n, char *filename);

#endif
