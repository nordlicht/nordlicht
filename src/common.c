#include <stdarg.h>
#include "common.h"

void error(char *message, ...) {
   fprintf(stderr, "nordlicht: ");
   va_list arglist;
   va_start(arglist, message);
   vfprintf(stderr, message, arglist);
   va_end(arglist);
   fprintf(stderr, "\n");
}
