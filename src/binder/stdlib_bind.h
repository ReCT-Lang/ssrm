
// AUTO GENERATED!
#ifndef STDLIB_BIND_H
#define STDLIB_BIND_H

#include "scope.h"

void push_stdlib(binder_context* binder, scope_object* object);


#ifdef STDLIB_BIND_IMPL

#include <stdlib.h>
#include <string.h>

static string copy_string(binder_context* context, string src) {
    string str = (string)msalloc(context->alloc_stack, (int)strlen(src) + 1);
    strcpy(str, src);
    return str;
}

typedef struct {
    const char* name;
    const int private;
    const scope_object_type type;
    const int* children;
} scope_object_template;

const int child_list_0[] = {-1};
const int child_list_1[] = {2,3,-1};
const int child_list_2[] = {-1};
const int child_list_3[] = {-1};

const scope_object_template templates[4] = {
    { .name = "ref", .private = 0, .type = SCOPE_OBJECT_CLASS,.children = child_list_0 },{ .name = "string", .private = 0, .type = SCOPE_OBJECT_CLASS,.children = child_list_1 },{ .name = "GetLength", .private = 0, .type = SCOPE_OBJECT_FUNCTION,.children = child_list_2 },{ .name = "Empty", .private = 0, .type = SCOPE_OBJECT_VARIABLE,.children = child_list_3 }
};

const int root_objects[] = { 0, 1 };

static scope_object* construct_object(binder_context* binder, int id, scope_object* parent);

static scope_object* construct_object(binder_context* binder, int id, scope_object* parent) {
    scope_object_template template = templates[id];
    scope_object* o = new_scope_object(binder, template.type, NULL);
    
    o->parent = parent;
    o->name = copy_string(binder, (string)template.name);
    o->private = template.private;
    
    for(int i = 0; template.children[i] != -1; i++) {
        scope_object* child = construct_object(binder, template.children[i], o);
        object_list_push(binder, o->children, child);
    }
    
    return o;
}

void push_stdlib(binder_context* binder, scope_object* object) {
    for(int i = 0; i < 2; i++) {
        scope_object* child = construct_object(binder, root_objects[i], object);
        object_list_push(binder, object->children, child);
    }
}


#endif
#endif
