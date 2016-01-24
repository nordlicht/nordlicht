#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

char *error_message = NULL;

void error(const char *message, ...) {
    if (error_message != NULL) {
        free(error_message);
    }
    error_message = (char *) malloc(1000); // TODO
    va_list arglist;
    va_start(arglist, message);
    vsnprintf(error_message, 1000, message, arglist);
    va_end(arglist);
}

const char *get_error() {
    return error_message;
}
