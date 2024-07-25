#include "file_context.h"
#include <string.h>


file_context* create_file_context(string path) {
    memstack* stack = msnew();
    file_context* ctx = (file_context*)msalloc(stack, sizeof(file_context));
    ctx->alloc_stack = stack;
    ctx->path = msalloc(stack, strlen(path));
    strcpy(ctx->path, path);
    return ctx;
}

void destroy_file_context(file_context* context) {
    msfree(context->alloc_stack);
}