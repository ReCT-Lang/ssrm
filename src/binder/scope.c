#include "scope.h"
#include "util/log.h"

void object_list_push(binder_context* binder, scope_object_list* list, scope_object* object) {
    // We're out of space! Reallocate
    if(list->allocated <= list->length + 1) {
        list->allocated = list->allocated * 2 + 1; // The new size is twice as large + 1 to avoid any 0-lengths.
        scope_object** new_list = (scope_object**)msalloc(binder->alloc_stack, sizeof(scope_object*) * list->allocated);

        if(list->objects != NULL) // NULL check to be safe.
        {
            for (int i = 0; i < list->length; ++i) {
                new_list[i] = list->objects[i]; // Copy over data :)
            }
            msdealloc(binder->alloc_stack, list->objects); // Free the old one
        }

        list->objects = new_list;
    }
    list->objects[list->length] = object;
    list->length++;
}

scope_object_list* new_scope_object_list(binder_context* binder) {
    scope_object_list* list = (scope_object_list*)msalloc(binder->alloc_stack, sizeof(scope_object_list));

    list->allocated = 0;
    list->length = 0;
    list->objects = NULL;

    return list;
}

scope_object* new_scope_object(binder_context* binder, scope_object_type type, node* source) {
    scope_object* object = (scope_object*)msalloc(binder->alloc_stack, sizeof(scope_object));

    object->object_type = type;
    object->children = new_scope_object_list(binder);
    object->private = 0;
    object->parent = NULL;
    object->name = NULL;
    object->source = source;

    return object;
}

const char* get_scope_name(scope_object_type t) {
    switch (t) {
        case SCOPE_OBJECT_GLOBAL:
            return "SCOPE_OBJECT_GLOBAL";
        case SCOPE_OBJECT_PACKAGE:
            return "SCOPE_OBJECT_PACKAGE";
        case SCOPE_OBJECT_FUNCTION:
            return "SCOPE_OBJECT_FUNCTION";
        case SCOPE_OBJECT_VARIABLE:
            return "SCOPE_OBJECT_VARIABLE";
        case SCOPE_OBJECT_CLASS:
            return "SCOPE_OBJECT_CLASS";
        case SCOPE_OBJECT_STRUCT:
            return "SCOPE_OBJECT_STRUCT";
        case SCOPE_OBJECT_PARAMETER:
            return "SCOPE_OBJECT_PARAMETER";
        case SCOPE_OBJECT_TYPE:
            return "SCOPE_OBJECT_TYPE";
        default:
            return "???";
    }
}


static void print_scope_object_i(scope_object* object, int indent) {

    if(object == NULL) {
        printf("%*sNULL\n", indent * 4, "");
        return;
    }

    if(object->source == NULL)
        lprintf("%*s%s: %s - private: %i\n", indent * 4, "", get_scope_name(object->object_type),
           object->name, object->private);
    else
        lprintf("%*s%s: %s - private: %i -  %s [L: %i, C: %i]\n", indent * 4, "", get_scope_name(object->object_type),
               object->name, object->private, NODE_TYPE_NAMES[object->source->type],
               object->source->loc.line, object->source->loc.column);

    for (int i = 0; i < object->children->length; ++i) {
        print_scope_object_i(object->children->objects[i], indent + 1);
    }
}

void print_scope_object(scope_object* object) {
    print_scope_object_i(object, 0);
}

location scope_location(scope_object* object) {
    if(object == NULL)
        return (location) {0, 0};
    if(object->source == NULL)
        return (location) {0, 0};
}