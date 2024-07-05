#pragma once

#include "binder.h"
#include "ext.h"

// There are a number of different scope properties:

typedef enum scope_object_type {
    SCOPE_OBJECT_GLOBAL     = 0b00000001,
    SCOPE_OBJECT_PACKAGE    = 0b00000010,
    SCOPE_OBJECT_FUNCTION   = 0b00000100,
    SCOPE_OBJECT_VARIABLE   = 0b00001000,
    SCOPE_OBJECT_CLASS      = 0b00010000,
    SCOPE_OBJECT_STRUCT     = 0b00100000,
    SCOPE_OBJECT_PARAMETER  = 0b01000000,
    SCOPE_OBJECT_TYPE       = 0b10000000,

    SCOPE_OBJECT_NONE       = 0b00000000,
    SCOPE_OBJECT_ALL        = 0b11111111
} scope_object_type;

const char* get_scope_name(scope_object_type t);

typedef struct scope_object_list scope_object_list;
typedef struct scope_object scope_object;

typedef struct scope_object {
    scope_object_type object_type;
    scope_object_list* children;
    char* name;
    scope_object* parent;
    int private;
    node* source;
} scope_object;

scope_object* new_scope_object(binder_context* binder, scope_object_type type, node* source);

typedef struct scope_object_list {
    scope_object** objects;
    int length;
    int allocated;
} scope_object_list;
void object_list_push(binder_context* binder, scope_object_list* list, scope_object* object);
scope_object_list* new_scope_object_list(binder_context* binder);
location scope_location(scope_object* object);
scope_object* get_scope_child(scope_object* o, string name);

void print_scope_object(scope_object* object);