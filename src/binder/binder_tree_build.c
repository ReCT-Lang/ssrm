#include "binder.h"
#include <errors/error.h>
#include <string.h>

static scope_object* build_scope_object(binder_context* binder, node* node);
static scope_object_class* build_class(binder_context* binder, node_class_def* node);
static void build_body(scope_object_list* list, node_body* node);
static void build_perms(scope_object* object, permissions perms, const char* error, const char* error_id, location loc);

static string copy_string(binder_context* context, string src) {
    string str = (string)balloc(context, (int)strlen(src) + 1);
    strcpy(str, src);
    return str;
}


scope_object_list* binder_build_file(binder_context* binder, parser_context* parser) {
    scope_object_list* list = new_scope_object_list(binder);

    for (int i = 0; i < parser->node->children->length; ++i) {
        scope_object* object = build_scope_object(binder, parser->node->children->data[i]);
        if(object != NULL) {
            push_scope_object(list, object);
        }
    }

    return list;
}

scope_object* build_scope_object(binder_context* binder, node* node) {

    if(node->type == NODE_CLASS_DEF) {
        return (scope_object*)build_class(binder, as_node_class_def(node));
    }

    return NULL;
}

static scope_object_class* build_class(binder_context* binder, node_class_def* node) {
    scope_object_class* class_object = new_so_class(binder);
    class_object->origin_node = node;

    build_perms((scope_object*)class_object, node->flags,
                "Class declaration cannot be both public and private at the same time!", "RCT3010", node->loc);

    class_object->name = copy_string(binder, node->name);

    build_body(class_object->children, node->body);

    return class_object;
}

// It's a body builder.... Get it?
static void build_body(scope_object_list* list, node_body* node) {
    for (int i = 0; i < node->children->length; ++i) {
        scope_object* object = build_scope_object(list->binder, node->children->data[i]);
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