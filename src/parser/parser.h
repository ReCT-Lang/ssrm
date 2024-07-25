#ifndef PARSER_H
#define PARSER_H

#include <memstack.h>
#include "nodes.h"
#include "../lexer/lexer.h"

typedef struct node_root node_root;

typedef struct parser_context {
    memstack* allocation_stack;
    node_root* node;
    lexer_context* lexer;
    int token_current;
    file_context* file;
} parser_context;

parser_context* parser_create(lexer_context* lexer);
void parser_destroy(parser_context* parser);

void parser_parse(parser_context* parser);
void* palloc(parser_context* context, int size);

#endif