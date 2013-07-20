#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "nordlicht.h"

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
    nordlicht *code = NULL;

    assert_fail(nordlicht_create(&code, 0, 100));
    assert_fail(nordlicht_create(&code, 1024, 0));
    assert_pass(nordlicht_create(&code, 1024, 100));
    assert(code != NULL);
    assert(nordlicht_progress(code) == 0);
    assert(nordlicht_is_done(code));

    assert_pass(nordlicht_free(code));
}

void test_input() {
    nordlicht *code;
    nordlicht_create(&code, 1024, 100);
    assert_pass(nordlicht_output(code, CODE));

    assert_pass(nordlicht_input(code, VID));
    assert_pass(nordlicht_input(code, VID));
    assert_pass(nordlicht_input(code, VID));
    usleep(100000);
    assert(!nordlicht_is_done(code));
    assert_pass(nordlicht_input(code, VID));

    assert_pass(nordlicht_free(code));
}

void test_output() {
    remove(CODE);
    assert(!file_exists(CODE));

    nordlicht *code;
    nordlicht_create(&code, 1024, 100);
    nordlicht_input(code, VID);

    nordlicht_output(code, CODE);
    usleep(100000);
    assert(file_exists(CODE));

    nordlicht_stop(code);
    remove(CODE);
    assert(!file_exists(CODE));


    nordlicht_free(code);
}

int main() {
    meta_test_assets();

    test_create();
    test_input();
    test_output();

    printf("All assertions passed. Yay!\n");
    return 0;
}
