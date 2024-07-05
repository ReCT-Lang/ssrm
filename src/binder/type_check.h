#pragma once

#include "binder.h"
#include "scope.h"

typedef struct {
    string* parts;
    scope_object* object;
    int package;

    struct meta {
        int allocated;
        int length;
    } meta;
} identifier_expanded_t;

void identifier_push(binder_context* binder, identifier_expanded_t* expanded, string part);
identifier_expanded_t* identifier_exp_new(binder_context* binder);

int type_check(binder_context* binder, scope_object* scope);