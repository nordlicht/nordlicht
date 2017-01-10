#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__
#include <stdlib.h> // for size_t

#ifndef NORDLICHT_API
#  ifdef _WIN32
#     if defined(NORDLICHT_BUILD_SHARED) /* build dll */
#         define NORDLICHT_API __declspec(dllexport)
#     elif !defined(NORDLICHT_BUILD_STATIC) /* use dll */
#         define NORDLICHT_API __declspec(dllimport)
#     else /* static library */
#         define NORDLICHT_API
#     endif
#  else
#     if __GNUC__ >= 4
#         define NORDLICHT_API __attribute__((visibility("default")))
#     else
#         define NORDLICHT_API
#     endif
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nordlicht nordlicht;

typedef enum nordlicht_style {
    NORDLICHT_STYLE_THUMBNAILS, // a row of thumbnails
    NORDLICHT_STYLE_HORIZONTAL, // compress frames to columns
    NORDLICHT_STYLE_VERTICAL, // compress frames to rows and rotate them counterclockwise
    NORDLICHT_STYLE_SLITSCAN, // take single columns, while moving to the right (and wrapping to the left)
    NORDLICHT_STYLE_MIDDLECOLUMN, // take the frames' middlemost column
    NORDLICHT_STYLE_SPECTROGRAM, // spectrogram of the first audio track (not all sample formats are supported yet)
    NORDLICHT_STYLE_THUMBNAILSTHIRD, // same as 'thumbnails', but only use the middle third
    NORDLICHT_STYLE_HORIZONTALTHIRD, // same as 'horizontal', but only use the middle third
    NORDLICHT_STYLE_VERTICALTHIRD, // same as 'vertical', but only use the middle third
    NORDLICHT_STYLE_SLITSCANTHIRD, // same as 'slitscan', but only use the middle third
    NORDLICHT_STYLE_LAST // just a marker so that we can count the number of available styles
} nordlicht_style;

typedef enum nordlicht_strategy {
    NORDLICHT_STRATEGY_FAST, // generate barcode in a single pass as fast as possible
    NORDLICHT_STRATEGY_LIVE, // generate a fast approximation first, good for live display
} nordlicht_strategy;

// Returns a description of the last error, or NULL if there was no error.
NORDLICHT_API const char *nordlicht_error();

// Allocate a new nordlicht of specified width and height, for a given video
// file. Use `nordlicht_free` to free the nordlicht again.
// Returns NULL on errors.
NORDLICHT_API nordlicht* nordlicht_init(const char *filename, const int width, const int height);

// Free a nordlicht.
NORDLICHT_API void nordlicht_free(nordlicht *n);

// Set the number of rows. Default is 1.
NORDLICHT_API int nordlicht_set_rows(nordlicht *n, const int rows);

// Specify where to start the nordlicht in the file, as a ratio between 0 and 1.
NORDLICHT_API int nordlicht_set_start(nordlicht *n, const float start);

// Specify where to end the nordlicht in the file, as a ratio between 0 and 1.
NORDLICHT_API int nordlicht_set_end(nordlicht *n, const float end);

// Set the output style of the nordlicht, see above. Default is NORDLICHT_STYLE_HORIZONTAL.
// To set multiple styles at once, use `nordlicht_set_styles`.
// Returns 0 on success. 
NORDLICHT_API int nordlicht_set_style(nordlicht *n, const nordlicht_style style);

// Set multiple output styles, which will be displayed on top of each other.
// Expects a pointer to an array of nordlicht_style-s of length `num_styles`.
// Returns 0 on success.
NORDLICHT_API int nordlicht_set_styles(nordlicht *n, const nordlicht_style *styles, const int num_styles);

// Set the generation strategy of the nordlicht, see above. Default is NORDLICHT_STRATEGY_FAST.
// Returns 0 on success.
NORDLICHT_API int nordlicht_set_strategy(nordlicht *n, const nordlicht_strategy strategy);

// Return a pointer to the nordlicht's internal buffer. You can use it to draw
// the barcode while it is generated. The pixel format is 32-bit BGRA.
NORDLICHT_API const unsigned char* nordlicht_buffer(const nordlicht *n);

// Replace the internal nordlicht's internal buffer. The data pointer is owned
// by the caller and must be freed after `nordlicht_free`. You can use this to
// render the nordlicht into caller-owned data structures, like mmap-ed files.
// The pixel format is 32-bit BGRA.
// Returns 0 on success.
NORDLICHT_API int nordlicht_set_buffer(nordlicht *n, unsigned char *data);

// Returns the size of this nordlicht's buffer in bytes.
NORDLICHT_API size_t nordlicht_buffer_size(const nordlicht *n);

// Generate the nordlicht in one pass. Use this function from a thread if you don't
// want to block execution. Calling this will freeze the nordlicht: "set"
// functions will fail.
// Returns 0 on success.
NORDLICHT_API int nordlicht_generate(nordlicht *n);

// Do one step of generation, which will be as small as possible. Use this
// if you don't want to start a seperate thread, but be aware that this
// function might still take too long for real-time applications. Calling this
// will freeze the nordlicht: "set" functions will fail.
// Returns 0 on success.
NORDLICHT_API int nordlicht_generate_step(nordlicht *n);

// Returns 1 if the nordlicht has been completely generated, and 0 otherwise.
NORDLICHT_API int nordlicht_done(const nordlicht *n);

// Returns a value between 0 and 1 indicating how much of the nordlicht has been
// generated.
NORDLICHT_API float nordlicht_progress(const nordlicht *n);

// Write the nordlicht to a PNG file. Returns 0 on success.
NORDLICHT_API int nordlicht_write(const nordlicht *n, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
