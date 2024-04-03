#include "binder.h"
#include <stdlib.h>
#include <string.h>
#include "scope.h"
#include <errors/error.h>

scope_object* binder_build_object(binder_context* binder, node* node, scope_object* parent);

static location get_scope_location(scope_object* o) {
    if(o->source == NULL)
        return (location) { 0, 0 };
    return o->source->loc;
}

static string copy_string(binder_context* context, string src) {
    string str = (string)msalloc(context->alloc_stack, (int)strlen(src) + 1);
    strcpy(str, src);
    return str;
}

static string read_value(binder_context* context, bind_ext_resolver resolver, bind_ext_object object) {
    int length = resolver.value_fetch(object, NULL);
    string s = (string)msalloc(context->alloc_stack, length + 1);
    resolver.value_fetch(object, s);
    return s;
}

binder_context* binder_create(bind_ext_resolver resolver) {
    binder_context* binder = (binder_context*)malloc(sizeof(binder_context));

    binder->resolver = resolver;
    binder->alloc_stack = msnew();
    binder->program_nodes = NULL;
    binder->program_node_count = 0;

    return binder;
}

void binder_mount(binder_context* binder, node_root* program_node, string filename) {
    // For now, we'll not bother about performant lists, since we won't do *that* many adds.
    // Like, one thousand isn't too damn bad

    binder_bound_program* new_array = msalloc(binder->alloc_stack, binder->program_node_count + 1);
    for (int i = 0; i < binder->program_node_count; ++i) {
        new_array[i] = binder->program_nodes[i];
    }

    binder_bound_program n;
    n.root = program_node;
    n.name = copy_string(binder, filename);

    msdealloc(binder->alloc_stack, binder->program_nodes);
    binder->program_nodes = new_array;
    binder->program_nodes[binder->program_node_count] = n;
    binder->program_node_count++;
}

scope_object* binder_build_ext(binder_context* binder, bind_ext_resolver resolver, bind_ext_object object, scope_object* parent) {
    bind_ext_object_kind kind = resolver.kind_fetch(object);

    if(kind == EXT_OBJECT_KIND_FUNCTION) {
        // Return a function
        scope_object* function = new_scope_object(binder, SCOPE_OBJECT_FUNCTION, NULL);
        function->name = read_value(binder, resolver, object);
        function->private = resolver.level_fetch(object) == EXT_ACCESS_LEVEL_PRIVATE;
        function->parent = parent;

        int values = resolver.count_fetch(object);

        for (int i = 0; i < values; ++i) {
            bind_ext_object value = resolver.object_fetch(object, i);
            bind_ext_object_kind value_kind = resolver.kind_fetch(value);

            // Parameters
            if(value_kind == EXT_OBJECT_KIND_PARAMETER) {
                scope_object* param = new_scope_object(binder, SCOPE_OBJECT_PARAMETER, NULL);
                param->name = read_value(binder, resolver, value);
                function->private = 0;
                function->parent = function;

                // We assume a child(or whetever else you want me to say)
                bind_ext_object p = resolver.object_fetch(value, 0);
                scope_object* param_t = new_scope_object(binder, SCOPE_OBJECT_TYPE, NULL);
                param->name = read_value(binder, resolver, p);
                function->private = 0;
                function->parent = param;

                object_list_push(binder, function->children, param);
            }
        }

        return function;
    }

    return NULL;
}

scope_object* binder_build_package(binder_context* binder, node_package_def* package_def) {
    scope_object* object = new_scope_object(binder, SCOPE_OBJECT_PACKAGE, (node*)package_def);

    object->private = 1;
    object->name = copy_string(binder, package_def->package_name);

    bind_ext_resolver resolver = binder->resolver;

    bind_ext_object package_obj = resolver.get_package(package_def->package_name);
    for (int i = 0; i < resolver.count_fetch(package_obj); ++i) {
        scope_object* obj = binder_build_ext(binder, resolver, resolver.object_fetch(package_obj, i), object);
        if(obj != NULL)
            object_list_push(binder, object->children, obj);
    }

    return object;
}

scope_object* binder_build_class(binder_context* binder, node_class_def* class_def, scope_object* parent) {
    scope_object* object = new_scope_object(binder, SCOPE_OBJECT_CLASS, (node*)class_def);
    object->parent = parent;
    object->name = copy_string(binder, class_def->name);
    object->private = (class_def->flags & PERMS_PRIVATE) || !(class_def->flags & PERMS_PUBLIC);

    for (int i = 0; i < class_def->body->children->length; ++i) {
        scope_object* o =
                binder_build_object(binder, class_def->body->children->data[i], object);
        if(o != NULL)
            object_list_push(binder, object->children, o);
    }

    return object;
}

scope_object* binder_build_struct(binder_context* binder, node_struct_def* struct_node, scope_object* parent) {
    scope_object* object = new_scope_object(binder, SCOPE_OBJECT_STRUCT, (node*)struct_node);
    object->parent = parent;
    object->name = copy_string(binder, struct_node->name);
    object->private = (struct_node->flags & PERMS_PRIVATE) || !(struct_node->flags & PERMS_PUBLIC);

    for (int i = 0; i < struct_node->body->children->length; ++i) {
        scope_object* o =
                binder_build_object(binder, struct_node->body->children->data[i], object);
        if(o != NULL)
            object_list_push(binder, object->children, o);
    }

    return object;
}

scope_object* binder_build_function(binder_context* binder, node_function_def* function_node, scope_object* parent) {

    scope_object* object = new_scope_object(binder, SCOPE_OBJECT_FUNCTION, (node*)function_node);
    object->parent = parent;
    object->name = copy_string(binder, function_node->name);
    object->private = (function_node->flags & PERMS_PRIVATE) || !(function_node->flags & PERMS_PUBLIC);

    if(function_node->body == NULL)
        return object;

    for (int i = 0; i < function_node->body->children->length; ++i) {
        scope_object* o =
                binder_build_object(binder, function_node->body->children->data[i], object);
        if(o != NULL)
            object_list_push(binder, object->children, o);
    }

    return object;
}

scope_object* binder_build_variable(binder_context* binder, node_variable_def* variable_def, scope_object* parent) {

    scope_object* object = new_scope_object(binder, SCOPE_OBJECT_VARIABLE, (node*)variable_def);
    object->parent = parent;
    object->name = copy_string(binder, variable_def->name);
    object->private = (variable_def->flags & PERMS_PRIVATE) || !(variable_def->flags & PERMS_PUBLIC);

    return object;
}

scope_object* binder_build_object(binder_context* binder, node* n, scope_object* parent) {

    if(n->type == NODE_PACKAGE_DEF)
        return binder_build_package(binder, as_node_package_def(n));
    if(n->type == NODE_CLASS_DEF)
        return binder_build_class(binder, as_node_class_def(n), parent);
    if(n->type == NODE_STRUCT_DEF)
        return binder_build_struct(binder, as_node_struct_def(n), parent);
    if(n->type == NODE_FUNCTION_DEF)
        return binder_build_function(binder, as_node_function_def(n), parent);
    if(n->type == NODE_VARIABLE_DEF)
        return binder_build_variable(binder, as_node_variable_def(n), parent);

    return NULL;
}



scope_object* binder_build_global(binder_context* binder, node_root* root) {
    scope_object* global_scope = new_scope_object(binder, SCOPE_OBJECT_GLOBAL, (node*)root);

    for (int i = 0; i < root->children->length; ++i) {
        scope_object* o = binder_build_object(binder, root->children->data[i], global_scope);
        if(o != NULL)
            object_list_push(binder, global_scope->children, o);
    }

    return global_scope;
}

static int validate_scope(binder_context* binder, scope_object* scope, scope_object_type allowed);

static int validate_class_scope(binder_context* binder, scope_object* class) {
    int failed = 0;
    for (int i = 0; i < class->children->length; ++i) {
        if(validate_scope(binder, class->children->objects[i], (SCOPE_OBJECT_ALL ^ SCOPE_OBJECT_GLOBAL)  ^ SCOPE_OBJECT_PACKAGE))
            failed = 1;
    }
    return failed;
}

static int validate_struct_scope(binder_context* binder, scope_object* class) {
    int failed = 0;
    for (int i = 0; i < class->children->length; ++i) {
        if(validate_scope(binder, class->children->objects[i], (SCOPE_OBJECT_FUNCTION | SCOPE_OBJECT_VARIABLE)))
            failed = 1;
    }
    return failed;
}

static int validate_global_scope(binder_context* binder, scope_object* global) {
    int failed = 0;
    for (int i = 0; i < global->children->length; ++i) {
        // Global scopes may contain anything(except global scopes), so we just go for it
        // By doing SCOPE_OBJECT_ALL and XOR-ing SCOPE_OBJECT_GLOBAL we basically set the flags to everything
        // but SCOPE_OBJECT_GLOBAL. Could also do `ALL & !GLOB` but I get complaints about simplifying to `& 0`
        if(validate_scope(binder, global->children->objects[i], SCOPE_OBJECT_ALL ^ SCOPE_OBJECT_GLOBAL))
            failed = 1;
    }
    return failed;
}

static int validate_function_scope(binder_context* binder, scope_object* global) {
    int failed = 0;
    for (int i = 0; i < global->children->length; ++i) {
        // Global scopes may contain anything(except global scopes), so we just go for it
        // By doing SCOPE_OBJECT_ALL and XOR-ing SCOPE_OBJECT_GLOBAL we basically set the flags to everything
        // but SCOPE_OBJECT_GLOBAL. Could also do `ALL & !GLOB` but I get complaints about simplifying to `& 0`
        if(validate_scope(binder, global->children->objects[i], SCOPE_OBJECT_VARIABLE))
            failed = 1;
    }
    return failed;
}

static int validate_package_scope(binder_context* binder, scope_object* global) {
    int failed = 0;
    for (int i = 0; i < global->children->length; ++i) {
        // Global scopes may contain anything(except global scopes), so we just go for it
        // By doing SCOPE_OBJECT_ALL and XOR-ing SCOPE_OBJECT_GLOBAL we basically set the flags to everything
        // but SCOPE_OBJECT_GLOBAL. Could also do `ALL & !GLOB` but I get complaints about simplifying to `& 0`
        if(validate_scope(binder, global->children->objects[i], SCOPE_OBJECT_VARIABLE | SCOPE_OBJECT_FUNCTION
            | SCOPE_OBJECT_CLASS | SCOPE_OBJECT_STRUCT))
            failed = 1;
    }
    return failed;
}

static int validate_scope(binder_context* binder, scope_object* scope, scope_object_type allowed) {
    // These will only be created in VERY SPECIFIC contexts
    if(scope->object_type == SCOPE_OBJECT_PARAMETER)
        return 0;
    if(scope->object_type == SCOPE_OBJECT_TYPE)
        return 0;

    if (!(scope->object_type & allowed)) { // Flag check!
        if(scope->parent == NULL)
            error_throw("RCT3001", get_scope_location(scope), "Attempted to define disallowed type %s", get_scope_name(scope->object_type));
        else
            error_throw("RCT3001",get_scope_location(scope), "Attempted to define disallowed type %s within %s", get_scope_name(scope->object_type),
                        get_scope_name(scope->parent->object_type));
        return 1;
    }

    if(scope->object_type == SCOPE_OBJECT_GLOBAL)
        return validate_global_scope(binder, scope);
    if(scope->object_type == SCOPE_OBJECT_CLASS)
        return validate_class_scope(binder, scope);
    if(scope->object_type == SCOPE_OBJECT_STRUCT)
        return validate_struct_scope(binder, scope);
    if(scope->object_type == SCOPE_OBJECT_FUNCTION)
        return validate_function_scope(binder, scope);
    if(scope->object_type == SCOPE_OBJECT_PACKAGE)
        return validate_package_scope(binder, scope);
    if(scope->object_type == SCOPE_OBJECT_VARIABLE)
        return 0;

    error_throw("RCT3001", get_scope_location(scope), "Attempted to define unhandled type %s", get_scope_name(scope->object_type));
    return 1;
}

static int validate_root_glob_scope(binder_context* binder, scope_object* global) {
    int failed = 0;
    for (int i = 0; i < global->children->length; ++i) {
        if(validate_global_scope(binder, global->children->objects[i]))
            failed = 1;
    }
    return failed;
}

static int validate_binary_expression(scope_object* object, node_binary_exp* binary) {
    error_throw("RCT3666", binary->loc, "Binaries are a no-no");
    return 1;
}

static int validate_unary_expression(scope_object* object, node_unary_exp* unary) {
    error_throw("RCT3666", unary->loc, "Unaries are a no-no");
    return 1;
}

static int validate_statement(scope_object* object, node* expression) {
    if(expression->type == NODE_BINARY_EXP)
        return validate_binary_expression(object, as_node_binary_exp(expression));
    if(expression->type == NODE_UNARY_EXP)
        return validate_unary_expression(object, as_node_unary_exp(expression));
    if(expression->type == NODE_LITERAL)
        return 0; // Literals are safe :D
    lprintf("Unexpected statement/expression node %s\n", NODE_TYPE_NAMES[expression->type]);
    return 1;
}

static int validate_expression_so(scope_object* object) {
    if(object->source == NULL) {
        lprintf("Could not find source for expression node!?\n");
        return 0;
    }

    if(object->object_type == SCOPE_OBJECT_FUNCTION) {
        node_function_def* function = as_node_function_def(object->source);
        error_throw("RCT3666", function->loc, "Functions are scary");
        // TODO: Functions are scary.
        return 0;
    } else if(object->object_type == SCOPE_OBJECT_VARIABLE) {
        node_variable_def* variable = as_node_variable_def(object->source);
        if(variable->default_value)
            validate_statement(object, variable->default_value);
    }

    lprintf("Could not validate node!\n");
    return 0;
}

static int check_expressions(scope_object* object) {
    if(object->object_type == SCOPE_OBJECT_FUNCTION) {
        return validate_expression_so(object);
    } else if (object->object_type == SCOPE_OBJECT_VARIABLE) {
        return validate_expression_so(object);
    } else {
        int failed = 0;
        for (int i = 0; i < object->children->length; ++i) {
            if(check_expressions(object->children->objects[i]))
                failed = 1;
        }
        return failed;
    }
    return 0;
}

int binder_validate(binder_context* binder) {

    // 1. Build the "program tree".
    scope_object* root_scope = new_scope_object(binder, SCOPE_OBJECT_GLOBAL, NULL);
    root_scope->name = "__$binder_root_node";
    for (int i = 0; i < binder->program_node_count; ++i) {
        scope_object* object = binder_build_global(binder, binder->program_nodes[i].root);
        object->name = binder->program_nodes[i].name;
        object_list_push(binder, root_scope->children, object);
    }

    print_scope_object(root_scope);

    // 2. Validate program tree/object tree
    if(validate_root_glob_scope(binder, root_scope)) {
        return 1;
    }

    // 3. Validate all expressiosn
    if(check_expressions(root_scope)) {
        return 1;
    }
}

void binder_destroy(binder_context* binder) {
    msfree(binder->alloc_stack);
    free(binder);
}