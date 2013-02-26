#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "mediabarcode.h"

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
    mediabarcode *code = NULL;

    assert_fail(mbc_create(&code, 0, 100));
    assert_fail(mbc_create(&code, 1024, 0));
    assert_pass(mbc_create(&code, 1024, 100));
    assert(code != NULL);
    assert(mbc_progress(code) == 0);
    assert(mbc_is_done(code));

    assert_pass(mbc_free(code));
}

void test_input() {
    mediabarcode *code;
    mbc_create(&code, 1024, 100);
    assert_pass(mbc_output(code, CODE));

    assert_pass(mbc_input(code, VID));
    assert_pass(mbc_input(code, VID));
    assert_pass(mbc_input(code, VID));
    usleep(100000);
    assert(!mbc_is_done(code));
    assert_pass(mbc_input(code, VID));

    assert_pass(mbc_free(code));
}

void test_output() {
    remove(CODE);
    assert(!file_exists(CODE));

    mediabarcode *code;
    mbc_create(&code, 1024, 100);
    mbc_input(code, VID);

    mbc_output(code, CODE);
    usleep(100000);
    assert(file_exists(CODE));

    mbc_stop(code);
    remove(CODE);
    assert(!file_exists(CODE));


    mbc_free(code);
}

int main() {
    meta_test_assets();

    test_create();
    test_input();
    test_output();

    printf("All assertions passed. Yay!\n");
    return 0;
}
