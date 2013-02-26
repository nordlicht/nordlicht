#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "vidcode.h"

#define assert_pass(expr) assert((expr) == 0)
#define assert_fail(expr) assert((expr) != 0)

#define VID "vid.avi"
#define CODE "code.ppm"

bool file_exists(char *filename) {
    return access(filename, F_OK) != -1;
}

void meta_test_assets() {
    assert(file_exists(VID));
}

void test_create() {
    vidcode *code = NULL;

    assert_fail(vidcode_create(&code, 0, 100));
    assert_fail(vidcode_create(&code, 1024, 0));
    assert_pass(vidcode_create(&code, 1024, 100));
    assert(code != NULL);
    assert(vidcode_progress(code) == 0);
    assert(vidcode_is_done(code));

    assert_pass(vidcode_free(code));
}

void test_input() {
    vidcode *code;
    vidcode_create(&code, 1024, 100);
    assert_pass(vidcode_output(code, CODE));

    assert_pass(vidcode_input(code, VID));
    assert_pass(vidcode_input(code, VID));
    assert_pass(vidcode_input(code, VID));
    usleep(100000);
    assert(!vidcode_is_done(code));
    assert_pass(vidcode_input(code, VID));

    assert_pass(vidcode_free(code));
}

void test_output() {
    remove(CODE);
    assert(!file_exists(CODE));

    vidcode *code;
    vidcode_create(&code, 1024, 100);
    vidcode_input(code, VID);

    vidcode_output(code, CODE);
    usleep(100000);
    assert(file_exists(CODE));

    vidcode_stop(code);
    remove(CODE);
    assert(!file_exists(CODE));


    vidcode_free(code);
}

int main() {
    meta_test_assets();

    test_create();
    test_input();
    test_output();

    printf("All assertions passed. Yay!\n");
    return 0;
}
