import json

print("Generating standard library bind tree")
output_f = open("stdlib_bind.h", "w")

stdlib_flattened = []

json_file = open("stdlib_bind.json")
stdlib_data = json.load(json_file)
json_file.close()


def push_node(name, data, parent):
    global stdlib_flattened

    identifier = len(stdlib_flattened)

    flattened = {
        "name": name,
        "type": data["type"],
        "private": data["private"],
        "parent": parent,
        "children": [],
        "id": identifier
    }

    stdlib_flattened.append(flattened)

    # Push all of our children
    if "children" in data:
        for _n in data["children"].keys():
            flattened["children"].append(push_node(_n, data["children"][_n], identifier))
    flattened["children"].append(-1)

    stdlib_flattened[identifier] = flattened

    return identifier


root_objects = []

for n in stdlib_data.keys():
    root_objects.append(push_node(n, stdlib_data[n], -1))

template_strings = []

for o in stdlib_flattened:
    s = f'{{ .name = "{o["name"]}", .private = {1 if o["private"] else 0}, .type = {o["type"]},'
    s += f'.children = child_list_{str(o["id"])} }}'

    template_strings.append(s)

newline = "\n"

output_string = f'''
// AUTO GENERATED!
#ifndef STDLIB_BIND_H
#define STDLIB_BIND_H

#include "scope.h"

void push_stdlib(binder_context* binder, scope_object* object);


#ifdef STDLIB_BIND_IMPL

#include <stdlib.h>
#include <string.h>

static string copy_string(binder_context* context, string src) {{
    string str = (string)msalloc(context->alloc_stack, (int)strlen(src) + 1);
    strcpy(str, src);
    return str;
}}

typedef struct {{
    const char* name;
    const int private;
    const scope_object_type type;
    const int* children;
}} scope_object_template;

{newline.join([
    f"const int child_list_{str(o['id'])}[] = {{{','.join([str(c) for c in o['children']])}}};" for o in stdlib_flattened
])}

const scope_object_template templates[{str(len(stdlib_flattened))}] = {{
    {",".join(template_strings)}
}};

const int root_objects[] = {{ {', '.join([str(o) for o in root_objects])} }};

static scope_object* construct_object(binder_context* binder, int id, scope_object* parent);

static scope_object* construct_object(binder_context* binder, int id, scope_object* parent) {{
    scope_object_template template = templates[id];
    scope_object* o = new_scope_object(binder, template.type, NULL);
    
    o->parent = parent;
    o->name = copy_string(binder, (string)template.name);
    o->private = template.private;
    
    for(int i = 0; template.children[i] != -1; i++) {{
        scope_object* child = construct_object(binder, template.children[i], o);
        object_list_push(binder, o->children, child);
    }}
    
    return o;
}}

void push_stdlib(binder_context* binder, scope_object* object) {{
    for(int i = 0; i < {str(len(root_objects))}; i++) {{
        scope_object* child = construct_object(binder, root_objects[i], object);
        object_list_push(binder, object->children, child);
    }}
}}

'''

output_string += '''
#endif
#endif
'''

output_f.write(output_string)
output_f.close()
