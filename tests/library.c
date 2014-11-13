#include "cheat.h"
#include "nordlicht.h"

#define cheat_null(x) cheat_assert(NULL == (x))
#define cheat_not_null(x) cheat_assert(NULL != (x))
#define cheat_fail(x) cheat_assert(0 != (x))
#define cheat_ok(x) cheat_assert(0 == (x))

CHEAT_DECLARE(
    nordlicht *n;
    char *v = "video.mp4";
)

CHEAT_TEST(testfile_exists,
    cheat_assert(-1 != access(v, F_OK));
)

CHEAT_TEST(invalid_input_file,
    cheat_null(nordlicht_init(NULL, 100, 100));
    cheat_null(nordlicht_init("", 100, 100));
    cheat_null(nordlicht_init("\0", 100, 100));
    cheat_null(nordlicht_init(".", 100, 100));
    cheat_null(nordlicht_init("..", 100, 100));
    cheat_null(nordlicht_init("..", 100, 100));
    cheat_null(nordlicht_init("nonexistant_file.123", 100, 100));
)

CHEAT_TEST(valid_input_file,
    cheat_null(nordlicht_init(NULL, 100, 100));
    cheat_null(nordlicht_init("", 100, 100));
    cheat_null(nordlicht_init("\0", 100, 100));
    cheat_null(nordlicht_init(".", 100, 100));
    cheat_null(nordlicht_init("..", 100, 100));
    cheat_null(nordlicht_init("..", 100, 100));
    cheat_null(nordlicht_init("nonexistant_file.123", 100, 100));
)

CHEAT_TEST(invalid_size,
    cheat_null(nordlicht_init(v, 0, 100));
    cheat_null(nordlicht_init(v, 100, 0));
    cheat_null(nordlicht_init(v, 0, 0));
    cheat_null(nordlicht_init(v, -100, 100));
    cheat_null(nordlicht_init(v, 100, -100));
    cheat_null(nordlicht_init(v, INT_MIN, INT_MIN));
)


CHEAT_TEST(valid_size,
    cheat_not_null(nordlicht_init(v, 1, 1));
    cheat_not_null(nordlicht_init(v, INT_MAX, INT_MAX));
)

CHEAT_TEST(invalid_output,
    n = nordlicht_init(v, 1, 100);
    cheat_not_null(n);
    cheat_fail(nordlicht_write(n, NULL));
    cheat_fail(nordlicht_write(n, ""));
    cheat_fail(nordlicht_write(n, "\0"));
    cheat_fail(nordlicht_write(n, "."));
    cheat_fail(nordlicht_write(n, ".."));
    cheat_fail(nordlicht_write(n, v));
    nordlicht_free(n);
)

CHEAT_TEST(style,
    n = nordlicht_init(v, 1, 100);
    cheat_not_null(n);
    nordlicht_style styles[1] = {NORDLICHT_STYLE_HORIZONTAL};
    cheat_ok(nordlicht_set_style(n, styles, 1));
    styles[0] = NORDLICHT_STYLE_VERTICAL;
    cheat_ok(nordlicht_set_style(n, styles, 1));
    styles[0] = 10000000;
    cheat_fail(nordlicht_set_style(n, styles, 1));
    nordlicht_free(n);
)

CHEAT_TEST(strategy,
    n = nordlicht_init(v, 1, 100);
    cheat_not_null(n);
    cheat_ok(nordlicht_set_strategy(n, NORDLICHT_STRATEGY_FAST));
    cheat_ok(nordlicht_set_strategy(n, NORDLICHT_STRATEGY_LIVE));
    cheat_fail(nordlicht_set_strategy(n, 2));
    cheat_fail(nordlicht_set_strategy(n, 1000000));
    cheat_fail(nordlicht_set_strategy(n, -1));
    nordlicht_free(n);
)

CHEAT_TEST(buffer,
    const unsigned char *buffer = NULL;
    n = nordlicht_init(v, 2, 100);
    cheat_not_null(n);
    buffer = nordlicht_buffer(n);
    cheat_not_null(buffer);
    cheat_assert(2*100*4 == nordlicht_buffer_size(n));
    unsigned char *buffer2 = NULL;
    cheat_fail(nordlicht_set_buffer(n, buffer2));
    buffer2 = malloc(nordlicht_buffer_size(n));
    cheat_ok(nordlicht_set_buffer(n, buffer2));
    cheat_ok(nordlicht_set_buffer(n, buffer2));
    cheat_ok(nordlicht_set_buffer(n, buffer2));
    buffer = nordlicht_buffer(n);
    cheat_assert(buffer == buffer2);
    nordlicht_free(n);
    free(buffer2);
)

CHEAT_TEST(complete_run,
    unsigned char *buffer2 = NULL;
    n = nordlicht_init(v, 1, 100);
    cheat_not_null(n);
    cheat_ok(nordlicht_progress(n));
    cheat_ok(nordlicht_generate(n));
    cheat_assert(1 == nordlicht_progress(n));
    buffer2 = malloc(nordlicht_buffer_size(n));
    cheat_fail(nordlicht_set_buffer(n, buffer2));
    nordlicht_free(n);
    free(buffer2);
)
