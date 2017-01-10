#include "cheat.h"
#include "nordlicht.h"

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

#define cheat_null(x) cheat_assert(NULL == (x))
#define cheat_not_null(x) cheat_assert(NULL != (x))
#define cheat_fail(x) cheat_assert(0 != (x))
#define cheat_ok(x) cheat_assert(0 == (x))

CHEAT_DECLARE(
    nordlicht *n;

    int tool(char *args) {
        char c[200];
        snprintf(c, 200, "./nordlicht %s >/dev/null 2>/dev/null", args);
        return system(c);
    }
)

CHEAT_SET_UP(
    n = NULL;
)

CHEAT_TEAR_DOWN(
    if (n != NULL) {
        nordlicht_free(n);
    }
)

CHEAT_TEST(testfile_exists,
    cheat_assert(-1 != access("video.mp4", F_OK));
)

CHEAT_TEST(invalid_input_file,
    cheat_null(nordlicht_init(NULL, 100, 100));
    cheat_null(nordlicht_init("", 100, 100));
    cheat_null(nordlicht_init("\0", 100, 100));
    cheat_null(nordlicht_init(".", 100, 100));
    cheat_null(nordlicht_init("..", 100, 100));
    cheat_null(nordlicht_init("/", 100, 100));
    cheat_null(nordlicht_init("\\", 100, 100));
    cheat_null(nordlicht_init("nonexistent_file.123", 100, 100));
    cheat_null(nordlicht_init("video.mp4.", 100, 100));
)

CHEAT_TEST(invalid_size,
    cheat_null(nordlicht_init("video.mp4", 0, 100));
    cheat_null(nordlicht_init("video.mp4", 100, 0));
    cheat_null(nordlicht_init("video.mp4", 0, 0));
    cheat_null(nordlicht_init("video.mp4", -1, 1));
    cheat_null(nordlicht_init("video.mp4", 1, -1));
    cheat_null(nordlicht_init("video.mp4", -100, 100));
    cheat_null(nordlicht_init("video.mp4", 100, -100));
    cheat_null(nordlicht_init("video.mp4", INT_MIN, INT_MIN));
    cheat_null(nordlicht_init("video.mp4", 100001, 100001));
    cheat_null(nordlicht_init("video.mp4", 100001, 1));
    cheat_null(nordlicht_init("video.mp4", 1, 100001));
)


CHEAT_TEST(valid_size,
    cheat_not_null(nordlicht_init("video.mp4", 1, 1));
    cheat_not_null(nordlicht_init("video.mp4", 100, 100));
    cheat_not_null(nordlicht_init("video.mp4", 100000, 100000));
    cheat_not_null(nordlicht_init("video.mp4", 100000, 1));
    cheat_not_null(nordlicht_init("video.mp4", 1, 100000));
)

CHEAT_TEST(rows,
    n = nordlicht_init("video.mp4", 100, 100);
    cheat_not_null(n);
    cheat_ok(nordlicht_set_rows(n, 1));
    cheat_ok(nordlicht_set_rows(n, 100));
    cheat_fail(nordlicht_set_rows(n, 101));
    cheat_fail(nordlicht_set_rows(n, 0));
    cheat_fail(nordlicht_set_rows(n, -1));
    cheat_fail(nordlicht_set_rows(n, INT_MIN));
    cheat_fail(nordlicht_set_rows(n, INT_MAX));
)

CHEAT_TEST(invalid_output,
    n = nordlicht_init("video.mp4", 100, 100);
    cheat_not_null(n);
    cheat_fail(nordlicht_write(n, NULL));
    cheat_fail(nordlicht_write(n, ""));
    cheat_fail(nordlicht_write(n, "\0"));
    cheat_fail(nordlicht_write(n, "."));
    cheat_fail(nordlicht_write(n, ".."));
    cheat_fail(nordlicht_write(n, "/"));
    cheat_fail(nordlicht_write(n, "video.mp4"));
)

CHEAT_TEST(style,
    n = nordlicht_init("video.mp4", 100, 100);
    cheat_not_null(n);
    cheat_ok(nordlicht_set_style(n, NORDLICHT_STYLE_HORIZONTAL));
    cheat_ok(nordlicht_set_style(n, NORDLICHT_STYLE_VERTICAL));
    cheat_ok(nordlicht_set_style(n, NORDLICHT_STYLE_LAST-1));
    cheat_fail(nordlicht_set_style(n, -1));
    cheat_fail(nordlicht_set_style(n, INT_MAX));
    cheat_fail(nordlicht_set_style(n, INT_MIN));
)

CHEAT_TEST(styles,
    n = nordlicht_init("video.mp4", 100, 2);
    cheat_not_null(n);
    nordlicht_style styles[3];
    styles[0] = NORDLICHT_STYLE_HORIZONTAL;
    styles[1] = NORDLICHT_STYLE_VERTICAL;
    cheat_ok(nordlicht_set_styles(n, styles, 2));
    styles[0] = NORDLICHT_STYLE_LAST-1;
    cheat_ok(nordlicht_set_styles(n, styles, 2));
    styles[0] = INT_MAX;
    cheat_fail(nordlicht_set_styles(n, styles, 2));
    styles[0] = INT_MIN;
    cheat_fail(nordlicht_set_styles(n, styles, 2));
    styles[2] = NORDLICHT_STYLE_HORIZONTAL;
    cheat_fail(nordlicht_set_styles(n, styles, 3));
)

CHEAT_TEST(strategy,
    n = nordlicht_init("video.mp4", 100, 100);
    cheat_not_null(n);
    cheat_ok(nordlicht_set_strategy(n, NORDLICHT_STRATEGY_FAST));
    cheat_ok(nordlicht_set_strategy(n, NORDLICHT_STRATEGY_LIVE));
    cheat_fail(nordlicht_set_strategy(n, 2));
    cheat_fail(nordlicht_set_strategy(n, -1));
    cheat_fail(nordlicht_set_strategy(n, INT_MAX));
    cheat_fail(nordlicht_set_strategy(n, INT_MIN));
)

CHEAT_TEST(buffer,
    const unsigned char *buffer = NULL;
    n = nordlicht_init("video.mp4", 2, 100);
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
    free(buffer2);
)

CHEAT_TEST(generate_step,
    n = nordlicht_init("video.mp4", 1, 100);
    cheat_assert(0 == nordlicht_progress(n));
    cheat_assert(!nordlicht_done(n));
    cheat_ok(nordlicht_set_start(n, 0.5));
    cheat_assert(0 == nordlicht_generate_step(n));
    cheat_assert(!nordlicht_done(n));
    cheat_fail(nordlicht_set_start(n, 0.5));
    cheat_assert(0 == nordlicht_generate_step(n));
    cheat_assert(0 == nordlicht_generate_step(n));
    cheat_ok(nordlicht_generate(n));
    cheat_assert(nordlicht_done(n));
    cheat_assert(0 == nordlicht_generate_step(n));
    cheat_assert(nordlicht_done(n));
)

CHEAT_TEST(complete_run,
    unsigned char *buffer2 = NULL;
    n = nordlicht_init("video.mp4", 1, 100);
    cheat_not_null(n);
    cheat_ok(nordlicht_progress(n));
    cheat_ok(nordlicht_generate(n));
    cheat_assert(1 == nordlicht_progress(n));
    buffer2 = malloc(nordlicht_buffer_size(n));
    cheat_fail(nordlicht_set_buffer(n, buffer2));
    free(buffer2);
)

CHEAT_TEST(tool_argument_parsing,
    cheat_ok(tool("--help"));
    cheat_ok(tool("--version"));
    cheat_fail(tool(""));
    cheat_fail(tool("--fubar"));
    cheat_fail(tool("-1"));
    cheat_fail(tool("one.mp4 two.mp4"));
    cheat_fail(tool("does_not_exist.mp4"));
)

CHEAT_TEST(tool_size,
    cheat_ok(tool("video.mp4 -w 1 -h 1"));
    cheat_ok(tool("video.mp4 -w 1 -h 100000"));
    cheat_fail(tool("video.mp4 -w 1 -h 100001"));
    cheat_fail(tool("video.mp4 -w huuuge"));
    cheat_fail(tool("video.mp4 -w 0"));
    cheat_fail(tool("video.mp4 -h 0"));
    cheat_fail(tool("video.mp4 -w ''"));
    cheat_fail(tool("video.mp4 -h ''"));
    cheat_fail(tool("video.mp4 -w -100"));
    cheat_fail(tool("video.mp4 -h -100"));
    cheat_fail(tool("video.mp4 -w 1.1"));
    cheat_fail(tool("video.mp4 -h 1.1"));
    cheat_fail(tool("video.mp4 -w 1,1"));
    cheat_fail(tool("video.mp4 -h 1,1"));
)

CHEAT_TEST(tool_rows,
    cheat_ok(tool("video.mp4 -w 1 -h 10 -r 1"));
    cheat_ok(tool("video.mp4 -w 1 -h 10 -r 10"));
    cheat_fail(tool("video.mp4 -w 1 -h 10 -r 11"));
    cheat_fail(tool("video.mp4 -w 1 -h 10 -r 0"));
    cheat_fail(tool("video.mp4 -w 1 -h 10 -r -1"));
    cheat_fail(tool("video.mp4 -w 1 -h 10 -r -100000000000"));
    cheat_fail(tool("video.mp4 -w 1 -h 10 -r 100000000000"));
    cheat_ok(tool("video.mp4 -s horizontal+vertical -w 1 -h 10 -r 5"));
    cheat_fail(tool("video.mp4 -s horizontal+vertical -w 1 -h 10 -r 6"));
)

CHEAT_TEST(tool_output,
    cheat_ok(tool("video.mp4 -w 1"));
    cheat_assert(-1 != access("video.mp4.nordlicht.png", F_OK));
    cheat_ok(tool("video.mp4 -w 1 -o ünîç⌀də.png"));
    cheat_assert(-1 != access("ünîç⌀də.png", F_OK));
    cheat_fail(tool("video.mp4 -w 1 -o video.mp4"));
    cheat_fail(tool("video.mp4 -w 1 -o ''"));
    cheat_fail(tool("video.mp4 -w 1 -o ."));
    cheat_fail(tool("video.mp4 -w 1 -o .."));
    cheat_fail(tool("video.mp4 -w 1 -o /"));
    cheat_ok(tool("video.mp4 -w 1 -o timebar.bgra"));
    // fall back to PNG in these two cases:
    cheat_ok(tool("video.mp4 -w 1 -o timebar.abc"));
    cheat_ok(tool("video.mp4 -w 1 -o timebar"));
)

CHEAT_TEST(tool_style,
    cheat_ok(tool("video.mp4 -w 1 -s vertical"));
    cheat_ok(tool("video.mp4 -w 1 -s horizontal"));
    cheat_fail(tool("video.mp4 -h 1 -s horizontal+vertical"));
    cheat_fail(tool("video.mp4 -h 2 -s horizontal+vertical+horizontal"));
    cheat_ok(tool("video.mp4 -h 2 -s horizontal+vertical"));
    cheat_fail(tool("video.mp4 -s horizontal+"));
    cheat_fail(tool("video.mp4 -s +"));
    cheat_fail(tool("video.mp4 -s ++"));
    cheat_fail(tool("video.mp4 -s horizontal++horizontal"));
    cheat_fail(tool("video.mp4 -s nope"));
    cheat_fail(tool("video.mp4 -s ''"));
    cheat_fail(tool("video.mp4 -s"));
)

CHEAT_TEST(tool_region,
    cheat_ok(tool("video.mp4 -w 1 --start=0.5"));
    cheat_ok(tool("video.mp4 -w 1 --end=0.5"));
    cheat_ok(tool("video.mp4 -w 1 --start=0.1 --end=0.2"));
    cheat_ok(tool("video.mp4 -w 1 --start=0 --end=1"));
    cheat_fail(tool("video.mp4 -w 1 --start=-1"));
    cheat_fail(tool("video.mp4 -w 1 --start=1"));
    cheat_fail(tool("video.mp4 -w 1 --start=2"));
    cheat_fail(tool("video.mp4 -w 1 --end=-1"));
    cheat_fail(tool("video.mp4 -w 1 --end=0"));
    cheat_fail(tool("video.mp4 -w 1 --end=2"));
    cheat_fail(tool("video.mp4 -w 1 --start=0.2 --end=0.1"));
    cheat_fail(tool("video.mp4 -w 1 --start=0.5 --end=0.5"));
)
