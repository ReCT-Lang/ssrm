#include "binder.h"
#include <errors/error.h>
#include <string.h>

typedef enum {
    DATA_CLASS =        0b00000001,
    DATA_STRUCT =       0b00000010,
    DATA_VARIABLE =     0b00000100,
    DATA_FUNCTION =     0b00001000,
    DATA_STATEMENT =    0b00010000,
    DATA_PACKAGE =      0b00100000,

    DATA_NONE = 0,
    DATA_ALL = 0xFF
} allowed_data;

static scope_object* build_scope_object(binder_context* binder, node* node, allowed_data allowed_data, scope_object* parent);
static scope_object_class* build_class(binder_context* binder, node_class_def* node);
static scope_object_struct* build_struct(binder_context* binder, node_struct_def* node);
static scope_object_variable* build_variable(binder_context* binder, node_variable_def* node);
static scope_object_function* build_function(binder_context* binder, node_function_def* node);
static scope_object_package* build_package(binder_context* binder, node_package_def* node);
static void build_body(scope_object_list* list, node_body* node, allowed_data allowed_data, scope_object* parent);
static void build_perms(scope_object* object, permissions perms, const char* error, const char* error_id, location loc);

static string copy_string(binder_context* context, string src) {
    string str = (string)balloc(context, (int)strlen(src) + 1);
    strcpy(str, src);
    return str;
}


scope_object_list* binder_build_file(binder_context* binder, parser_context* parser) {
    scope_object_list* list = new_scope_object_list(binder);
    scope_object_package* makeshift_object = new_so_package(binder);
    makeshift_object->name = copy_string(binder, parser->node->file);
    makeshift_object->origin_node = (node*)parser->node;
    for (int i = 0; i < parser->node->children->length; ++i) {
        scope_object* object = build_scope_object(binder, parser->node->children->data[i], DATA_ALL, (scope_object*)makeshift_object);
        if(object != NULL) {
            push_scope_object(list, object);
        }
    }

    return list;
}

scope_object* build_scope_object(binder_context* binder, node* node, allowed_data allowed, scope_object* parent) {

    if(node->type == NODE_CLASS_DEF) {
        if(allowed & DATA_CLASS)
            return (scope_object*)build_class(binder, as_node_class_def(node));
        error_throw(ERR_DECL_DISALLOWED, node->loc, "Class declaration not allowed inside %s!", SCOPE_OBJECT_TYPES_LEG[parent->object_type]);
        return NULL;
    }

    if(node->type == NODE_STRUCT_DEF) {
        if(allowed & DATA_CLASS)
            return (scope_object*)build_struct(binder, as_node_struct_def(node));
        error_throw(ERR_DECL_DISALLOWED, node->loc, "Struct declaration not allowed inside %s!", SCOPE_OBJECT_TYPES_LEG[parent->object_type]);
        return NULL;
    }

    if(node->type == NODE_VARIABLE_DEF) {
        if(allowed & DATA_CLASS)
            return (scope_object*)build_variable(binder, as_node_variable_def(node));
        error_throw(ERR_DECL_DISALLOWED, node->loc, "Variable declaration not allowed inside %s!", SCOPE_OBJECT_TYPES_LEG[parent->object_type]);
        return NULL;
    }

    if(node->type == NODE_FUNCTION_DEF) {
        if(allowed & DATA_CLASS)
            return (scope_object*)build_function(binder, as_node_function_def(node));
        error_throw(ERR_DECL_DISALLOWED, node->loc, "Variable declaration not allowed inside %s!", SCOPE_OBJECT_TYPES_LEG[parent->object_type]);
        return NULL;
    }

    if(node->type == NODE_PACKAGE_DEF) {
        if(allowed & DATA_PACKAGE)
            return (scope_object*)build_package(binder, as_node_package_def(node));
        error_throw(ERR_DECL_DISALLOWED, node->loc, "Package inclusion not allowed inside %s!", SCOPE_OBJECT_TYPES_LEG[parent->object_type]);
        return NULL;
    }

    error_throw(ERR_NODE_UNEXPECTED, node->loc, "Unexpected %s within %s %s!", NODE_TYPE_NAMES[node->type], SCOPE_OBJECT_TYPES_LEG[parent->object_type], parent->name);

    return NULL;
}

static scope_object_class* build_class(binder_context* binder, node_class_def* node) {
    scope_object_class* class_object = new_so_class(binder);
    class_object->origin_node = node;

    build_perms((scope_object*)class_object, node->flags,
                "Class declaration cannot be both public and private at the same time!", ERR_ACCESS_DUALITY, node->loc);

    class_object->name = copy_string(binder, node->name);

    build_body(class_object->children, node->body, DATA_FUNCTION | DATA_VARIABLE | DATA_CLASS, (scope_object*)class_object);

    return class_object;
}

// It's a body builder.... Get it?
static void build_body(scope_object_list* list, node_body* node, allowed_data allowed, scope_object* parent) {
    for (int i = 0; i < node->children->length; ++i) {
        scope_object* object = build_scope_object(list->binder, node->children->data[i], allowed, parent);
        if(object != NULL) {
            push_scope_object(list, object);
        }
    }
}

static void build_perms(scope_object* object, permissions perms, const char* error, const char* error_id, location loc) {
    if(perms & PERMS_PUBLIC) {
        object->access |= ACCESS_PUBLIC;
    }
    if(perms & PERMS_PRIVATE) {
        object->access |= ACCESS_PRIVATE;
    }
    if(perms & PERMS_STATIC) {
        object->access |= ACCESS_STATIC;
    } else {
        object->access |= ACCESS_INSTANCE;
    }

    // If we're not public nor private
    if(!(object->access & (ACCESS_PUBLIC | ACCESS_PRIVATE))) {
        object->access |= ACCESS_PRIVATE; // We default to private
    }

    if((object->access & ACCESS_PUBLIC) && (object->access & ACCESS_PRIVATE)) {
        error_throw(error_id, loc, error);
    }
}

static scope_object_struct* build_struct(binder_context* binder, node_struct_def* node) {
    scope_object_struct* struct_object = new_so_struct(binder);
    struct_object->origin_node = node;

    build_perms((scope_object*)struct_object, node->flags,
                "Struct declaration cannot be both public and private at the same time!", ERR_ACCESS_DUALITY, node->loc);

    struct_object->name = copy_string(binder, node->name);

    build_body(struct_object->children, node->body, DATA_VARIABLE, (scope_object*)struct_object);

    return struct_object;
}

static scope_object_variable* build_variable(binder_context* binder, node_variable_def* node) {
    scope_object_variable* variable_object = new_so_variable(binder);
    variable_object->name = copy_string(binder, node->name);
    build_perms((scope_object*)variable_object, node->flags,
                "Variable declaration cannot be both public and private at the same time!", ERR_ACCESS_DUALITY, node->loc);
    return variable_object;
}

static scope_object_function* build_function(binder_context* binder, node_function_def* node) {
    scope_object_function* function_object = new_so_function(binder);
    function_object->name = copy_string(binder, node->name);
    build_perms((scope_object*)function_object, node->flags,
                "Function declaration cannot be both public and private at the same time!", ERR_ACCESS_DUALITY, node->loc);
    return function_object;
}

static scope_object_package* build_package(binder_context* binder, node_package_def* node) {
    if(binder->resolver == NULL) {
        error_throw(ERR_RESOLVER_MISSING, node->loc, "Binder context does not have any package resolver!");
        return NULL;
    }
    scope_object* package_object = binder->resolver(binder, NULL, node->package_name);
    if(package_object == NULL || package_object->object_type != SCOPE_OBJECT_PACKAGE) {
        error_throw(ERR_PACKAGE_NOTFOUND, node->loc, "Could not find package '%s'!", node->package_name);
        return NULL;
    }
    return (scope_object_package*)package_object;
}