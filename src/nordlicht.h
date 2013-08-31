#ifndef INCLUDE_nordlicht_h__
#define INCLUDE_nordlicht_h__

typedef struct nordlicht nordlicht;

// Allocate a new nordlicht of specific width and height. Use
// `nordlicht_free` to free it again.
nordlicht* nordlicht_create(int width, int height);

// Free a nordlicht.
int nordlicht_free(nordlicht *n);

// Set input video file. All codecs supported by ffmpeg should work.
int nordlicht_set_input(nordlicht *n, char *file_path);

// Set ouput file. As for now, only PNG files are supported. Plan is to
// overload this function to support direct generation to in-memory data
// structures.
int nordlicht_set_output(nordlicht *n, char *file_path);

// Do one "step" of generation, producing a usable but possibly incomplete
// output. Returns a float between 0 and 1 representing how much of the
// nordlicht was generated. A value of 1 means the nordlicht is done.
float nordlicht_step(nordlicht *n);

#endif
