#include "type_check.h"

scope_object* __impl_node_expression_evaluate_type_node_unary_exp(node_unary_exp* n) { return NULL; }
scope_object* __impl_node_expression_evaluate_type_node_binary_exp(node_binary_exp* n) { return NULL; }
scope_object* __impl_node_expression_evaluate_type_node_function_call(node_function_call* n) { return NULL; }
scope_object* __impl_node_expression_evaluate_type_node_literal(node_literal* n) { return NULL; }
scope_object* __impl_node_expression_evaluate_type_node_make(node_make* n) { return NULL; }


int type_check(binder_context* binder, scope_object* scope) {
    // Okay. So. Type checking logic.
    // How do we do this?
    return 0;
}