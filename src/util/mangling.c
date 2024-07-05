#include "mangling.h"


mangle mangle_identifier(memstack* alloc_stack, node_identifier* identifier, scope_object_type object_type) {
    // This will "mangle" names in the most logical fashion imaginable, i.e:
    // We just take the "full name" of the identifier.

    // The exception would be variables.

    if(identifier->type == NODE_VARIABLE_DEF) {

    }
}