# Wacky code that generates all node types & functions.
# This is pretty much required.

import json

print("Generating nodes")
output_f = open("nodes.h", "w")
output = '''// AUTO GENERATED!
#ifndef NODES_H
#define NODES_H

#include "parser.h"
#include <lexer/location.h>
#include <util/log.h>

typedef char* string;
typedef struct parser_context parser_context;

typedef enum permissions {
    PERMS_PUBLIC = 1,
    PERMS_PRIVATE = 2,
    PERMS_STATIC = 4,
    PERMS_EXTERN = 8,
    PERMS_UNSAFE = 16,
    PERMS_NONE = 0
} permissions;

typedef enum operators {
    OP_NONE,
    OP_ADD,
    OP_SUBTRACT,
    OP_DIVIDE,
    OP_MULTIPLY
} operators;

typedef enum {
\tNODE_NULL,
\t'''

defs = open("nodes.json")
definitions = json.load(defs)
defs.close()

output += ',\n\t'.join([k.upper() for k in definitions.keys()])
output += "\n} node_type;\n\n"

output += '''
typedef struct node {
    node_type type;
    location loc;
} node;

typedef struct {
    int length;
    int allocated;
    node** data;
} node_list;

void list_push(parser_context* context, node_list* list, node* data);

extern string NODE_TYPE_NAMES[''' + str(len(definitions) + 1) + '''];

'''

# Forwards declarations
for t in definitions.keys():
    output += "typedef struct " + t + " " + t + ";\n"

output += "\n\n\n"

for t in definitions.keys():
    data = definitions[t]
    output += "typedef struct " + t + "{\n"
    output += "\tnode_type type;\n"
    output += "\tlocation loc;\n"
    for fieldName in data.keys():
        dataType = data[fieldName]
        if dataType == "node_list":
            dataType += "*"
        if dataType == "node":
            dataType += "*"
        if dataType == t:
            dataType = "struct " + dataType + "*"
        if dataType in definitions:
            dataType += "*"
        output += "\t" + dataType + " " + fieldName + ";\n"
    output += "} " + t + ";\n\n"
    output += t + "* new_" + t + "(parser_context* parser);\n\n"
    output += t + "* as_" + t + "(node* n);\n\n"

output += "void print_node(node* node, const char* name, int indent);\n\n"
output += "#ifdef NODES_PRINT_IMPL\n\n"

output += '''
node_list* new_node_list(parser_context* context) {
    node_list* list = (node_list*)palloc(context, sizeof(node_list));
    list->length = 0;
    list->allocated = 0;
    list->data = NULL;
    return list;
};

static void print_node_list(const char* name, node_list* list, int indent) {
    if(list->length == 0) {
        lprintf("%*s%s: NULL\\n", indent * 4, "", name);
        return;
    }
    lprintf("%*s%s:\\n", indent * 4, "", name);
    for (int i = 0; i < list->length; ++i) {
        print_node(list->data[i], NULL, indent + 1);
    }
}

static void print_string(const char* name, string s, int indent) {
    lprintf("%*s%s: \\"%s\\"\\n", indent * 4, "", name, s);
}

static void print_int(const char* name, int v, int indent) {
    lprintf("%*s%s: \\"%i\\"\\n", indent * 4, "", name, v);
}\n\n'''

for t in definitions.keys():
    output += "void print_" + t + "(" + t + "* node, const char* name, int indent);\n"
output += "\n\n"

for t in definitions.keys():
    # Generate node type functions such as print & new
    output += t + "* new_" + t + "(parser_context* parser) {\n"
    output += "\t" + t + "* data = (" + t + "*)palloc(parser, sizeof(" + t + "));\n"
    output += "\t" + "memset(data, 0, sizeof(" + t + "));\n"
    output += "\t" + "data->type = " + t.upper() + ";\n"

    for f in definitions[t].keys():
        if definitions[t][f] == "node_list":
            output += "\tdata->" + f + " = new_node_list(parser);\n"

    output += "\t" + "return data;\n"
    output += "};\n\n"

    output += t + "* as_" + t + "(node* n) {\n"
    output += "\treturn n->type == " + t.upper() + " ? (" + t + "*)n : NULL;\n}\n\n"

    output += "void print_" + t + "(" + t + "* n, const char* name, int indent) {\n"
    output += "\tif(n == NULL)\n"
    output += "\t\treturn print_node((node*)n, name, indent);\n"
    output += "\tif(name == NULL)\n"
    output += '\t\tlprintf("%*s' + t.upper() + ' [L: %i, C: %i]\\n", indent * 4, "", n->loc.line, n->loc.column);\n'
    output += '\telse\n'
    output += '\t\tlprintf("%*s%s: ' + t.upper() + ' [L: %i, C: %i]\\n", indent * 4, "", name, n->loc.line, n->loc.column);\n'

    fields = definitions[t]

    for name in fields.keys():
        if fields[name] == "node_list":
            output += "\tprint_node_list(\"" + name + "\", n->" + name + ", indent + 1);\n"
        elif fields[name] == "string":
            output += "\tprint_string(\"" + name + "\", n->" + name + ", indent + 1);\n"
        elif fields[name] == "int":
            output += "\tprint_int(\"" + name + "\", n->" + name + ", indent + 1);\n"
        elif fields[name] == "permissions":  # Perms is just an int for now.
            output += "\tprint_int(\"" + name + "\", n->" + name + ", indent + 1);\n"
        elif fields[name] == "operators":  # Same for operators
            output += "\tprint_int(\"" + name + "\", n->" + name + ", indent + 1);\n"
        elif fields[name] == "node":
            output += "\tprint_node(n->" + name + ", \"" + name + "\", indent + 1);\n"
        elif fields[name] in definitions:
            output += "\tprint_" + fields[name] + "(n->" + name + ", \"" + name + "\", indent + 1);\n"

    output += "}\n\n"

output += '''void print_node(node* node, const char* name, int indent)
{
    if(node == NULL) {
        if(name != NULL)
            lprintf("%*s%s: NULL\\n", indent * 4, "", name);
        else
            lprintf("%*sNULL\\n", indent * 4, "");
        return;
    }
    
    switch (node->type) {
        case NODE_NULL:
            break;
'''

for t in definitions.keys():
    output += "\t\tcase " + t.upper() + ":\n"
    output += "\t\t\tprint_" + t + "((" + t + "*)node, name, indent);\n"
    output += "\t\t\tbreak;\n"

output += '''   }
}

string NODE_TYPE_NAMES[''' + str(len(definitions) + 1) + '''] = {
"NODE_NULL",
'''

output += ',\n'.join(['"' + x.upper() + '"' for x in definitions.keys()])

output += '''
};

'''

output += "#endif\n\n#endif\n"

output_f.write(output)
output_f.close()
