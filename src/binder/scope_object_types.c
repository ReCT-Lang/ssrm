#include "scope.h"
#include <util/types.h>

const string SCOPE_OBJECT_TYPES[6] = {
        "SCOPE_OBJECT_NULL",
        "SCOPE_OBJECT_CLASS",
        "SCOPE_OBJECT_FUNCTION",
        "SCOPE_OBJECT_VARIABLE",
        "SCOPE_OBJECT_STRUCT",
        "SCOPE_OBJECT_PACKAGE"
};

const string SCOPE_OBJECT_TYPES_LEG[6] = {
        "null",
        "class",
        "function",
        "variable",
        "struct",
        "package"
};