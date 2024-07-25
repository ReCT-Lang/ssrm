#ifndef SCOPE_H
#define SCOPE_H

#include <parser/nodes.h>
#include "binder.h"

// The default allocated size for scope_list
// 32 indices at 64-bit amounts to 256 bytes / 0.25KB / 0,000244141MB / 0,000000238GB, so I think we can afford it.
#ifndef SCOPE_LIST_INIT_ALLOCATED
#define SCOPE_LIST_INIT_ALLOCATED (32)
#endif

// The expansion rate for scope_list
// When it runs out of space it'll reallocate into size (allocated * SCOPE_LIST_EXPAND_RATE)
// 2 is probably okay, but fractions work too(1.5f is common as well)
#ifndef SCOPE_LIST_EXPAND_RATE
#define SCOPE_LIST_EXPAND_RATE (2)
#endif

#ifndef SCOPE_STACK_SIZE
#define SCOPE_STACK_SIZE (2048)
#endif

#ifndef SCOPE_STACK_MARKERS
#define SCOPE_STACK_MARKERS (2048)
#endif

// I fucking hate forwards declarations like these.
// Like why the fuck? It's being defined in <parser/nodes.h> but for
// WHATEVER the fuck reason I have to define it again because C is
// just dumb or whatever.
typedef struct node node;
typedef struct node_class_def node_class_def;
typedef struct node_function_def node_function_def;
typedef struct node_variable_def node_variable_def;
typedef struct node_struct_def node_struct_def;
typedef struct node_package_def node_package_def;
typedef struct binder_context binder_context;

typedef struct scope_object_list scope_object_list;

typedef struct node_identifier node_identifier;

typedef enum scope_object_type {
    SCOPE_OBJECT_NULL,
    SCOPE_OBJECT_CLASS,
    SCOPE_OBJECT_FUNCTION,
    SCOPE_OBJECT_VARIABLE,
    SCOPE_OBJECT_STRUCT,
    SCOPE_OBJECT_PACKAGE,
} scope_object_type;

extern const string SCOPE_OBJECT_TYPES[6];
extern const string SCOPE_OBJECT_TYPES_LEG[6];

typedef enum scope_object_access {
    ACCESS_NONE     = 0,
    ACCESS_PUBLIC   = 0b0001,
    ACCESS_PRIVATE  = 0b0010,
    ACCESS_STATIC   = 0b0100,
    ACCESS_INSTANCE = 0b1000
} scope_object_access;

// The root scope object type.
// Scope objects are things like classes, variables and functions.
// Packages as well.
typedef struct scope_object {
    scope_object_type object_type;
    scope_object_access access;
    string name;
    node* origin_node;
    void* user;
} scope_object;

// There are a couple of different scope objects that exist:
// - class
// - function
// - variable
// - struct
// - package
typedef struct scope_object_class {
    scope_object_type object_type;
    scope_object_access access;
    string name;
    node_class_def* origin_node;
    void* user;
    scope_object_list* children;
} scope_object_class;

typedef struct scope_object_function {
    scope_object_type object_type;
    scope_object_access access;
    string name;
    node_function_def* origin_node;
    void* user;
} scope_object_function;

typedef struct scope_object_variable {
    scope_object_type object_type;
    scope_object_access access;
    string name;
    node_variable_def* origin_node;
    void* user;
} scope_object_variable;

typedef struct scope_object_struct {
    scope_object_type object_type;
    scope_object_access access;
    string name;
    node_struct_def* origin_node;
    void* user;
    scope_object_list* children;
} scope_object_struct;

typedef struct scope_object_package {
    scope_object_type object_type;
    scope_object_access access;
    string name;
    node_package_def* origin_node;
    void* user;
    scope_object_list* children;
} scope_object_package;

typedef struct scope_object_list {
    scope_object** objects;
    size_t size;
    size_t allocated;
    binder_context* binder;
} scope_object_list;

scope_object_list* new_scope_object_list(binder_context* binder);
void push_scope_object(scope_object_list* list, scope_object* object);

scope_object_class* new_so_class(binder_context* binder);
scope_object_struct* new_so_struct(binder_context* binder);
scope_object_function* new_so_function(binder_context* binder);
scope_object_variable* new_so_variable(binder_context* binder);
scope_object_package* new_so_package(binder_context* binder);

void print_scope_object(scope_object* object, int depth);

// The scope itself is just like a stack of scope objects.
// You push stuff onto the scope stack, mark the location and then
// once you're done you pop it all.
typedef struct scope {
    scope_object* object_stack[SCOPE_STACK_SIZE];
    int* stack_markers[SCOPE_STACK_MARKERS];
    int object_top;
    int marker_top;
} scope;

scope* scope_mark(scope* s); // Mark the current stack size, will be returned to when "scope_pop" is invoked.
scope* scope_push(scope_object* object); // Pushes an object onto the stack.
scope* scope_pop(scope* s); // Rewinds back to the latest mark and pops that as well
scope_object* scope_get(scope* s, node_identifier* identifier); // Gets an object from the scope stack, if it is available.
scope* scope_assign(scope* s, node_identifier* identifier); // Binds an identifier according to the current scope.

#endif