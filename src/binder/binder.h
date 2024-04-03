#pragma once
#include "ext.h"
#include <parser/nodes.h>
#include <memstack.h>

typedef struct binder_bound_program {
    node_root* root;
    char* name;
} binder_bound_program;

typedef struct binder_context {
    bind_ext_resolver resolver;
    memstack* alloc_stack;

    binder_bound_program* program_nodes;
    int program_node_count;
} binder_context;

binder_context* binder_create(bind_ext_resolver resolver);
void binder_mount(binder_context* binder, node_root* program_node, string filename);
int binder_validate(binder_context* binder);
void binder_destroy(binder_context* binder);