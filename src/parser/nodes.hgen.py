# A new system for generating the node types etc.
# This uses a custom "typescheme" language I made up just now.
# It is just line-by-line processed haha

import re


class NodeType:
    variables: dict[str, str] = {}
    functions: dict[str, str] = {}
    is_abstract: bool = False
    name: str = ""
    inherits: str = ""

    def __init__(self):
        # I have to do this or all NodeTypes will share the dicts for whatever reason
        self.variables = {}
        self.functions = {}

    def does_inherit(self, t : str) -> bool:
        if not self.inherits:
            return False
        if self.inherits == t:
            return True
        return type_table[self.inherits].does_inherit(t)


type_table: dict[str, NodeType] = {}
parsed = []

def parse_file(file):
    current_parsing_type: NodeType | None = None
    while True:
        line = file.readline()
        if not line:
            return
        line = line.strip()

        # Remove comments
        line = re.sub(r'#.*', "", line)
        if len(line) == 0:
            continue

        parts = [lp for lp in line.split(' ') if lp.strip()]

        if parts[0].lower() == "begin":
            current_parsing_type = NodeType()

            if parts[1] == "abstract":
                current_parsing_type.is_abstract = True
                current_parsing_type.name = parts[2]
                if len(parts) == 5:
                    current_parsing_type.inherits = parts[4]
            else:
                current_parsing_type.name = parts[1]
                if len(parts) == 4:
                    current_parsing_type.inherits = parts[3]

            continue

        if parts[0].lower() == "end":
            type_table[current_parsing_type.name] = current_parsing_type
            current_parsing_type = None
            continue

        if parts[0].lower() == "variable":
            current_parsing_type.variables[parts[1]] = parts[3]
            continue

        if parts[0].lower() == "function":
            current_parsing_type.functions[parts[1]] = parts[3]
            continue

        if parts[0].lower() == "include":
            if parts[1] in parsed:
                continue
            parsed.append(parts[1])
            included_file = open(parts[1], "r")
            parse_file(included_file)
            included_file.close()


input_file = open("nodes.tscm", "r")

parse_file(input_file)

# We now have the full node tree! Time to create a file haha

output_file = open("nodes.h", "w+")

output_file.write('''// Auto-generated from nodes.tscm and nodes.hgen.py
#ifndef NODES_H
#define NODES_H

#include "parser.h"
#include <lexer/location.h>
#include <util/log.h>
#include <util/types.h>
#include <binder/scope.h>

typedef struct parser_context parser_context;
typedef struct scope_object scope_object;

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
    NODE_NULL,
''')

# Alright, so, we need to sort the types.
# Any types not involved with inheritance go first

type_enum_values = []

for t in type_table.values():
    if t.inherits or t.is_abstract:
        continue
    type_enum_values.append(t.name.upper())


def push_inheritance(t):
    if t.name.upper() in type_enum_values:
        return

    type_enum_values.append("_" + t.name.upper() + "_START")
    type_enum_values.append(t.name.upper())

    for ct in type_table.values():
        if not ct.inherits == t.name:
            continue

        if ct.is_abstract:
            push_inheritance(ct)
        else:
            type_enum_values.append(ct.name.upper())

    type_enum_values.append("_" + t.name.upper() + "_END")


for t in type_table.values():
    if not t.is_abstract:
        continue
    push_inheritance(t)

output_file.write(',\n'.join(["\t" + t for t in type_enum_values]))
output_file.write('''
} node_type;

''')

# We then forwards-declare everything

for t in type_table:
    output_file.write("typedef struct " + t + " " + t + ";\n")

output_file.write('''

typedef struct {
    int length;
    int allocated;
    node** data;
} node_list;

void list_push(parser_context* context, node_list* list, node* data);

extern string NODE_TYPE_NAMES[''' + str(len(type_enum_values) + 1) + '''];
''')


def write_type_variables(node_type: NodeType):
    if node_type.inherits:
        write_type_variables(type_table[node_type.inherits])

    for v in node_type.variables:
        data_type = node_type.variables[v]
        if data_type == "node_list":
            data_type += "*"
        if data_type == t:
            data_type = "struct " + data_type + "*"
        if data_type in type_table:
            data_type += "*"
        output_file.write("\t " + data_type + " " + v + ";\n")


def write_type_functions(node_type: NodeType, root_type: NodeType):
    if node_type.inherits:
        write_type_functions(type_table[node_type.inherits], root_type)

    for f in node_type.functions:
        data_type = node_type.functions[f]
        if data_type == "node_list":
            data_type += "*"
        if data_type == t:
            data_type = "struct " + data_type + "*"
        if data_type in type_table:
            data_type += "*"
        if node_type == root_type:
            output_file.write(data_type + " " + node_type.name + "_" + f + "(" + node_type.name + "* node);\n")
        else:
            output_file.write(
                data_type + " __impl_" + node_type.name + "_" + f + "_" + root_type.name + "(" + root_type.name + "* node);\n")


def write_type_header(node_type: NodeType):
    output_file.write("typedef struct " + node_type.name + " {\n")
    write_type_variables(node_type)
    output_file.write("} " + node_type.name + ";\n")
    output_file.write(node_type.name + "* new_" + node_type.name + "(parser_context* parser);\n")
    output_file.write(node_type.name + "* as_" + node_type.name + "(node* n);\n")
    write_type_functions(node_type, node_type)
    output_file.write("\n\n")


# Write the typedefs
for t in type_table.values():
    write_type_header(t)

output_file.write('''
void print_node(node* node, const char* name, int indent);

#ifdef NODES_PRINT_IMPL

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
}\n\n''')

for t in type_table.keys():
    if t != "node":
        output_file.write("void print_" + t + "(" + t + "* node, const char* name, int indent);\n");
output_file.write("\n\n")

abstracts = []

for t in type_table.keys():
    # Generate node type functions such as print & new
    output_file.write(t + "* new_" + t + "(parser_context* parser) {\n")
    output_file.write("\t" + t + "* data = (" + t + "*)palloc(parser, sizeof(" + t + "));\n")
    output_file.write("\t" + "memset(data, 0, sizeof(" + t + "));\n")
    output_file.write("\t" + "data->type = " + t.upper() + ";\n")

    for f in type_table[t].variables:
        if type_table[t].variables[f] == "node_list":
            output_file.write("\tdata->" + f + " = new_node_list(parser);\n")

    output_file.write("\t" + "return data;\n")
    output_file.write("};\n\n")

    output_file.write(t + "* as_" + t + "(node* n) {\n")
    if type_table[t].is_abstract:
        output_file.write("\treturn (n->type > _" + t.upper() + "_START" + " && ")
        output_file.write("n->type < _" + t.upper() + "_END) ? (" + t + "*)n : NULL;\n}\n\n")
    else:
        output_file.write("\treturn n->type == " + t.upper() + " ? (" + t + "*)n : NULL;\n}\n\n")

    if t != "node":
        output_file.write("void print_" + t + "(" + t + "* n, const char* name, int indent) {\n")
        output_file.write("\tif(n == NULL)\n")
        output_file.write("\t\treturn print_node((node*)n, name, indent);\n")
        output_file.write("\tif(name == NULL)\n")
        output_file.write('\t\tlprintf("%*s' + t.upper() + ' [L: %i, C: %i]\\n", indent * 4, "", n->loc.line, n->loc.column);\n')
        output_file.write('\telse\n')
        output_file.write('\t\tlprintf("%*s%s: ' + t.upper() + ' [L: %i, C: %i]\\n", indent * 4, "", name, n->loc.line, n->loc.column);\n')

        fields = type_table[t].variables

        for name in fields.keys():
            if fields[name] == "node_list":
                output_file.write("\tprint_node_list(\"" + name + "\", n->" + name + ", indent + 1);\n")
            elif fields[name] == "string":
                output_file.write("\tprint_string(\"" + name + "\", n->" + name + ", indent + 1);\n")
            elif fields[name] == "int":
                output_file.write("\tprint_int(\"" + name + "\", n->" + name + ", indent + 1);\n")
            elif fields[name] == "permissions":  # Perms is just an int for now.
                output_file.write("\tprint_int(\"" + name + "\", n->" + name + ", indent + 1);\n")
            elif fields[name] == "operators":  # Same for operators
                output_file.write("\tprint_int(\"" + name + "\", n->" + name + ", indent + 1);\n")
            elif fields[name] == "node":
                output_file.write("\tprint_node(n->" + name + ", \"" + name + "\", indent + 1);\n")
            elif fields[name] in type_table:
                output_file.write("\tprint_" + fields[name] + "(n->" + name + ", \"" + name + "\", indent + 1);\n")

        output_file.write("}\n\n")

    if type_table[t].is_abstract:
        for f in type_table[t].functions:
            output_file.write(type_table[t].functions[f] + " " + t + "_" + f + "(" + t + "* n) {\n")
            output_file.write("\tswitch (n->type) {\n")
            for it in type_table.values():
                if it.does_inherit(t):
                    abstracts.append(type_table[t].functions[f] + " __impl_" + t + "_" + f + "_" + it.name + "(" + it.name + "* n) {}")
                    output_file.write("\t\tcase " + it.name.upper() + ":\n\t\t\treturn __impl_" + t + "_" + f + "_" + it.name + "(as_" + it.name + "((node*)n));\n")

            output_file.write("\t}\n")
            output_file.write("}\n\n")

output_file.write('''void print_node(node* node, const char* name, int indent)
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
''')

for t in type_table.keys():
    if t == "node":
        continue
    output_file.write("\t\tcase " + t.upper() + ":\n")
    output_file.write("\t\t\tprint_" + t + "((" + t + "*)node, name, indent);\n")
    output_file.write("\t\t\tbreak;\n")

output_file.write('''   }
}

string NODE_TYPE_NAMES[''' + str(len(type_enum_values) + 1) + '''] = {
    "NODE_NULL",
    ''')

output_file.write(',\n\t'.join(['"' + v + '"' for v in type_enum_values]))

output_file.write('''
};

#endif
#endif
/* NEEDED FUNCTION OVERRIDES:

''')

for a in abstracts:
    output_file.write(a + "\n")

output_file.write("\n*/")

output_file.close()
