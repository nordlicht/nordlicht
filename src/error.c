#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

char *error_message = NULL;

void error(char *message, ...) {
    if (error_message != NULL) {
        free(error_message);
    }
    error_message = malloc(1000); // TODO
    va_list arglist;
    va_start(arglist, message);
    vsnprintf(error_message, 1000, message, arglist);
    va_end(arglist);
}

char *get_error() {
    return error_message;
}
