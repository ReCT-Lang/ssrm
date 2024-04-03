#include "log.h"
#include <util/settings.h>
#include <stdio.h>
#include <stdarg.h>

void lprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if(log_verbal) {
        vprintf(fmt, args);
    }
    va_end(args);
}