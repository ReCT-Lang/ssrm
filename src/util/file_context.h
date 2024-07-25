#ifndef FILE_CONTEXT_H
#define FILE_CONTEXT_H

#include <memstack.h>
#include "types.h"

// Allocate object of type _t at file _f
#define f_alloc(_f, _t) ((_t*)msalloc(_f->alloc_stack, sizeof(_t)))
// Allocate array of type _t with size _c at file _f
#define f_calloc(_f, _t, _c) ((_t*)msalloc(_f->alloc_stack, sizeof(_t) * _c))

typedef struct file_context {
    memstack* alloc_stack;
    string path;
} file_context;

file_context* create_file_context(string path);
void destroy_file_context(file_context* context);

#endif