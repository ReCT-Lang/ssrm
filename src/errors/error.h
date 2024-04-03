#pragma once
#include "../lexer/location.h"

typedef const char* error_code;

typedef struct error {
    error_code code;
    char* string;
    location loc;
} error;

void error_throw(error_code code, location loc, const char* fmt, ...);

// Get errors thrown so far. Pass in NULL to just get the length.
// This makes a copy of all errors, so you need to clean up them yourself.
int enumerate_errors(error* list);

void allow_error_print(int enabled);

void free_error(error err);

// Delete currently stored errors
void dispose_errors();