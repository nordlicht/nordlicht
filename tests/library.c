#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <unistd.h>
#include "nordlicht.h"

#define fail(call) assert(0 != (call))
#define pass(call) assert(0 == (call))
#define null(call) assert(NULL == (call))

int file_exists(char *filename) {
    return access(filename, F_OK) != -1;
}

int main(void) {
    nordlicht *n;
    char *VID = "video.mp4";

    // test environment
    assert(1);
    assert(!0);
    fail(-1);
    pass(0);
    assert(file_exists(VID));

    // invalid input
    null(nordlicht_init(NULL, 100, 100));
    null(nordlicht_init("", 100, 100));
    null(nordlicht_init("\0", 100, 100));
    null(nordlicht_init(".", 100, 100));
    null(nordlicht_init("..", 100, 100));
    null(nordlicht_init("nonexistant_file.123", 100, 100));

    // invalid size
    null(nordlicht_init(VID, 0, 100));
    null(nordlicht_init(VID, 100, 0));
    null(nordlicht_init(VID, 0, 0));
    null(nordlicht_init(VID, -100, 100));
    null(nordlicht_init(VID, 100, -100));
    null(nordlicht_init(VID, INT_MIN, INT_MIN));

    // valid size
    assert(nordlicht_init(VID, 1, 1));
    assert(nordlicht_init(VID, INT_MAX, INT_MAX));

    // invalid output
    n = nordlicht_init(VID, 1, 100);
    assert(n);
    fail(nordlicht_write(n, NULL));
    fail(nordlicht_write(n, ""));
    fail(nordlicht_write(n, "\0"));
    fail(nordlicht_write(n, "."));
    fail(nordlicht_write(n, ".."));
    fail(nordlicht_write(n, VID));
    nordlicht_free(n);

    // style
    n = nordlicht_init(VID, 1, 100);
    assert(n);
    pass(nordlicht_set_style(n, NORDLICHT_STYLE_HORIZONTAL));
    pass(nordlicht_set_style(n, NORDLICHT_STYLE_VERTICAL));
    fail(nordlicht_set_style(n, 2));
    fail(nordlicht_set_style(n, 1000000));
    fail(nordlicht_set_style(n, -1));
    nordlicht_free(n);

    // buffer
    const unsigned char *buffer = NULL;
    n = nordlicht_init(VID, 2, 100);
    assert(n);
    buffer = nordlicht_buffer(n);
    assert(buffer);
    assert(2*100*4 == nordlicht_buffer_size(n));
    unsigned char *buffer2 = NULL;
    fail(nordlicht_set_buffer(n, buffer2));
    buffer2 = malloc(nordlicht_buffer_size(n));
    pass(nordlicht_set_buffer(n, buffer2));
    pass(nordlicht_set_buffer(n, buffer2));
    pass(nordlicht_set_buffer(n, buffer2));
    buffer = nordlicht_buffer(n);
    assert(buffer == buffer2);
    nordlicht_free(n);
    free(buffer2);

    // complete run
    n = nordlicht_init(VID, 1, 100);
    assert(n);
    assert(0 == nordlicht_progress(n));
    pass(nordlicht_generate(n));
    assert(1 == nordlicht_progress(n));
    fail(nordlicht_set_style(n, NORDLICHT_STYLE_HORIZONTAL));
    buffer2 = malloc(nordlicht_buffer_size(n));
    fail(nordlicht_set_buffer(n, buffer2));
    nordlicht_free(n);
    free(buffer2);

    printf("--- nordlicht library test suite passed. ---\n");
    return 0;
}
