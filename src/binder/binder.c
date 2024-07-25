#include "binder.h"

void* balloc(binder_context* context, size_t size) {
    return msalloc(context->stack, size);
}

binder_context* binder_create() {
    // The binder is responsible for allocating itself.
    // Sounds a bit whack but that's how it is.
    memstack* binder_stack = msnew();

    binder_context* binder = (binder_context*) msalloc(binder_stack, sizeof(binder_context));
    binder->stack = binder_stack;
    binder->resolver = NULL;

    return binder;
}

void binder_destroy(binder_context* binder) {
    msfree(binder->stack);
}

void binder_set_resolver(binder_context* binder, library_resolver resolver) {
    binder->resolver = resolver;
}