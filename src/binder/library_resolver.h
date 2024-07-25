#ifndef LIBRARY_RESOLVER_H
#define LIBRARY_RESOLVER_H

#include "scope.h"
#include <parser/nodes.h>
#include "binder.h"
#include <util/extras.h>

FDECL(node_identifier)
FDECL(binder_context)
FDECL(scope_object)

#define DECL_LIBRES typedef scope_object* (*library_resolver)(binder_context* binder, scope_object* parent, string name)

DECL_LIBRES;

#endif