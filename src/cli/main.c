#include <lexer/lexer.h>
#include <parser/parser.h>
#include <errors/error.h>
#include <binder/binder.h>
#include <string.h>
#include <stdlib.h>
#include "util/log.h"

void throw_exceptions() {
    int error_count = enumerate_errors(NULL);
    error* errors = (error*)calloc(sizeof(error), error_count);
    enumerate_errors(errors);

    for (int i = 0; i < error_count; ++i) {
        error e = errors[i];
        fprintf(stderr, "[ERR] [L: %u, C: %u] %s: %s\n", e.loc.line, e.loc.column, e.code, e.string);
    }
    free(errors);
    if(error_count > 0)
        exit(1);
}
typedef struct obj obj;

typedef struct obj {
    int* children;
    char* value;
    bind_ext_object_kind kind;
    bind_ext_access_level level;
} obj;

obj objects[] = {
        {.value = "NULL", .kind = EXT_OBJECT_KIND_GLOBAL_SCOPE,  .level = EXT_ACCESS_LEVEL_PRIVATE, .children = NULL},
        {.value = "sys", .kind = EXT_OBJECT_KIND_GLOBAL_SCOPE, .level = EXT_ACCESS_LEVEL_STATIC, .children = (int[]){2, -1}},
        {.value = "Print", .kind = EXT_OBJECT_KIND_FUNCTION, .level = EXT_ACCESS_LEVEL_STATIC, .children = (int[]){3, -1}},
        {.value = "fmt", .kind = EXT_OBJECT_KIND_PARAMETER, .level = EXT_ACCESS_LEVEL_STATIC, .children = (int[]){4, -1}},
        {.value = "std::string", .kind = EXT_OBJECT_KIND_TYPE, .level = EXT_ACCESS_LEVEL_STATIC, .children = NULL}
};

int count_fetch(bind_ext_object object) {
    if(objects[object].children == NULL) return 0;

    int c = 0;
    while (objects[object].children[c] != -1) c++;

    return c;
}

bind_ext_object object_fetch(bind_ext_object object, int index) {
    return objects[object].children[index];
}

int value_fetch(bind_ext_object object, char* buffer) {
    if(buffer != NULL) {
        strcpy(buffer, objects[object].value);
    }
    return (int)strlen(objects[object].value);
}

bind_ext_object_kind kind_fetch(bind_ext_object object) {
    return objects[object].kind;
}

bind_ext_access_level level_fetch(bind_ext_object object) {
    return objects[object].level;
}

bind_ext_object get_package(char* name) {
    if(strcmp(name, "sys") == 0)
        return 1;
    lprintf("Invalid package '%s!'\n", name);
    return 0;
}

parser_context* parse_file(FILE* in) {
    lexer_context* lexer = lexer_create();
    lexer_read(lexer, in);

    lexer_process(lexer);

    lprintf("Tokens:\n");
    for (int i = 0; i < lexer->token_count; ++i) {
        lprintf("| [L: %i, C: %i] %s: %s\n", lexer->tokens[i].loc.line, lexer->tokens[i].loc.column, TOKEN_NAMES[lexer->tokens[i].type], lexer->tokens[i].data);
    }
    lprintf("\n\n");

    throw_exceptions();

    parser_context* parser = parser_create(lexer);

    parser_parse(parser);

    lexer_destroy(lexer);

    lprintf("Parser Tree:\n\n");
    print_node((node*)parser->node, NULL, 0);
    lprintf("\n\n");

    return parser;
}

int main() {

    allow_error_print(1);

    bind_ext_resolver resolver = {};
    resolver.count_fetch = count_fetch;
    resolver.get_package = get_package;
    resolver.kind_fetch = kind_fetch;
    resolver.level_fetch = level_fetch;
    resolver.object_fetch = object_fetch;
    resolver.value_fetch = value_fetch;

    lprintf("Parsing test_simple.rct\n");
    FILE* in = fopen("src/r_test/test_simple.rct", "r");
    parser_context* test_simple = parse_file(in);
    fclose(in);

    lprintf("Parsing test_external.rct\n");
    in = fopen("src/r_test/test_external.rct", "r");
    parser_context* test_external = parse_file(in);
    fclose(in);

#ifdef DYNAMIC_STDLIB
    lprintf("Parsing stdlib.rct\n");
    in = fopen("stdlib_h/stdlib.rct", "r");
    parser_context* stdlib = parse_file(in);
    fclose(in);
#endif


    binder_context* binder = binder_create(resolver);

    binder_mount(binder, test_simple->node, "test_simple.rct");
    binder_mount(binder, test_external->node, "test_external.rct");
#ifdef DYNAMIC_STDLIB
    binder_mount(binder, stdlib->node, "__$stdlib");
#endif

    if(binder_validate(binder)) {
        // error!
        lprintf("Binder error!\n");
    }

    binder_destroy(binder);

    parser_destroy(test_simple);
    parser_destroy(test_external);

    throw_exceptions();
}