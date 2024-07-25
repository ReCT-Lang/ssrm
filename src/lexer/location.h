#pragma once
#include <util/types.h>
#include <util/file_context.h>

typedef struct location {
    file_context* context;
    unsigned int line;
    unsigned int column;
} location;