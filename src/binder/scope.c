#include "scope.h"
#include "stdio.h"
#include <util/log.h>
#include <string.h>
#include "stdlib.h"


scope_object_list* new_scope_object_list(binder_context* binder) {
    scope_object_list* list = b_alloc(binder, scope_object_list);
    list->allocated = SCOPE_LIST_INIT_ALLOCATED;
    list->objects = b_calloc(binder,  scope_object*, SCOPE_LIST_INIT_ALLOCATED);
    list->binder = binder;
    list->size = 0;
    return list;
}

void push_scope_object(scope_object_list* list, scope_object* object) {
    // It's all full. We need to reallocate.
    if(list->allocated == list->size) {
        size_t new_size = (size_t)(list->allocated * SCOPE_LIST_EXPAND_RATE);
        scope_object** new_buffer = b_calloc(list->binder, scope_object*, new_size);
        for (size_t i = 0; i < list->size; ++i) {
            new_buffer[i] = list->objects[i];
        }
        msfree((void*)list->objects);
        list->objects = new_buffer;
        list->allocated = new_size;
    }

    // Fairly easy to just push onto the list
    list->objects[list->size] = object;
    list->size++;
}

scope_object_class* new_so_class(binder_context* binder) {
    scope_object_class* object = b_alloc(binder, scope_object_class);
    object->object_type = SCOPE_OBJECT_CLASS;
    object->children = new_scope_object_list(binder);
    object->access = ACCESS_NONE;
    object->name = NULL;
    return object;
}

scope_object_struct* new_so_struct(binder_context* binder) {
    scope_object_struct* object = b_alloc(binder, scope_object_struct);
    object->object_type = SCOPE_OBJECT_STRUCT;
    object->children = new_scope_object_list(binder);
    object->origin_node = NULL;
    object->access = ACCESS_NONE;
    object->name = NULL;
    return object;
}

scope_object_function* new_so_function(binder_context* binder) {
    scope_object_function* object = b_alloc(binder, scope_object_function);
    object->object_type = SCOPE_OBJECT_FUNCTION;
    object->origin_node = NULL;
    object->access = ACCESS_NONE;
    object->name = NULL;
    return object;
}

scope_object_variable* new_so_variable(binder_context* binder) {
    scope_object_variable* object = b_alloc(binder, scope_object_variable);
    object->object_type = SCOPE_OBJECT_VARIABLE;
    object->origin_node = NULL;
    object->access = ACCESS_NONE;
    object->name = NULL;
    return object;
}

scope_object_package* new_so_package(binder_context* binder) {
    scope_object_package* object = b_alloc(binder, scope_object_package);
    object->object_type = SCOPE_OBJECT_PACKAGE;
    object->children = new_scope_object_list(binder);
    object->origin_node = NULL;
    object->access = ACCESS_NONE;
    object->name = NULL;
    return object;
}

#define log_i(_indent, ...) { for (int i = 0; i < _indent; i++) putchar('\t'); lprintf(__VA_ARGS__); }

void print_scope_object(scope_object* object, int depth) {
    int has_children = object->object_type == SCOPE_OBJECT_PACKAGE ||
                       object->object_type == SCOPE_OBJECT_STRUCT ||
                       object->object_type == SCOPE_OBJECT_CLASS;

    char access_string[2048] = {0};
    if (object->access & ACCESS_PRIVATE) {
        sprintf(access_string + strlen(access_string), "private ");
    }
    if (object->access & ACCESS_PUBLIC) {
        sprintf(access_string + strlen(access_string), "public ");
    }
    if (object->access & ACCESS_INSTANCE) {
        sprintf(access_string + strlen(access_string), "instance ");
    }
    if (object->access & ACCESS_STATIC) {
        sprintf(access_string + strlen(access_string), "static ");
    }

    log_i(depth, "%s%s %s\n", access_string, SCOPE_OBJECT_TYPES_LEG[object->object_type], object->name)
    if(has_children) {
        // Structs, packages and classes look the same at the raw binary level,
        // so we can safely cast to class without any worries.
        scope_object_class* childhaver = (scope_object_class*)object;
        log_i(depth + 1, "Children:\n");
        for (size_t i = 0; i < childhaver->children->size; ++i) {
            print_scope_object(childhaver->children->objects[i++], depth + 2);
        }
    }
}