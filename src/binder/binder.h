#ifndef BINDER_H
#define BINDER_H

#include "scope.h"
#include "binder.h"
#include <parser/parser.h>

// Allocate object of type _t at binder _b
#define b_alloc(_b, _t) ((_t*)balloc(_b, sizeof(_t)))
// Allocate array of type _t with size _c at binder _b
#define b_calloc(_b, _t, _c) ((_t*)balloc(_b, sizeof(_t) * _c))

typedef struct scope_object_list scope_object_list;
typedef struct parser_context parser_context;

typedef scope_object_list* built_file;

typedef struct binder_context {
    memstack* stack;
} binder_context;

binder_context* binder_create();
void binder_destroy(binder_context* binder);

scope_object_list* binder_build_file(binder_context* binder, parser_context* parser);

void* balloc(binder_context* context, size_t size);

#endif