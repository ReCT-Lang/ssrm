#include <string.h>
#include "parser.h"
#include "stdlib.h"
#include "nodes.h"
#include "../errors/error.h"

static node* parse_statement(parser_context* parser, int semicolon);
static node* parse_expression(parser_context* parser);

static void throw_invalid_token(token_t token) {
    if(token.data == NULL)
        error_throw("RCT2001", token.loc, "Unexpected token %s", TOKEN_NAMES[token.type]);
    else
        error_throw("RCT2001", token.loc, "Unexpected token %s (%s)", token.data, TOKEN_NAMES[token.type]);
}

void* palloc(parser_context* context, int size) {
    return msalloc(context->allocation_stack, size);
}

static string copy_string(parser_context* context, string src) {
    string str = (string)palloc(context, (int)strlen(src) + 1);
    strcpy(str, src);
    return str;
}

static token_t peek(parser_context* parser, int amt) {
    int index = parser->token_current + amt;
    if(index >= parser->lexer->token_count) {
        return (token_t) { .data = NULL, .type = TOKEN_EOF };
    }
    return parser->lexer->tokens[index];
}

static token_t current(parser_context* parser) {
    return peek(parser, 0);
}

static int at(parser_context* parser, token_type_e type) {
    return current(parser).type == type;
}

static void step(parser_context* parser, int amt) {
    parser->token_current += amt;
}

static token_t consume(parser_context* parser, token_type_e type) {
    token_t c = current(parser);
    step(parser, 1);
    if(c.type != type) {
        error_throw("RCT2002", current(parser).loc, "Unexpected token %s, expected %s",
                    TOKEN_NAMES[c.type], TOKEN_NAMES[type]);
    }
    return c;
}


parser_context* parser_create(lexer_context* lexer) {
    parser_context* context = (parser_context*)malloc(sizeof(parser_context));
    context->allocation_stack = msnew();
    context->lexer = lexer;
    context->token_current = 0;

    return context;
}

void parser_destroy(parser_context* parser) {
    msfree(parser->allocation_stack);
    free(parser);
}

static int unary_op_precedence(token_type_e type) {
    switch (type) {
        case TOKEN_ASSIGN_PLUS:
        case TOKEN_ASSIGN_MINUS:
        case TOKEN_OP_NOT:
            return 6;

        default:
            return 0;
    }
}

static int binary_op_precedence(token_type_e type) {
    switch (type) {
        case TOKEN_OP_STAR:
        case TOKEN_OP_SLASH:
        case TOKEN_OP_MOD:
            return 5;

        case TOKEN_OP_PLUS:
        case TOKEN_OP_MINUS:
            return 4;

        case TOKEN_OP_EQUALS:
        case TOKEN_OP_NOT_EQUALS:
        case TOKEN_OP_LT:
        case TOKEN_OP_GT:
        case TOKEN_OP_LT_EQUALS:
        case TOKEN_OP_GR_EQUALS: // TODO: Bitshifts
            return 3;

        // TODO: Ampersand
        // TODO: Pipes & Hats

        default:
            return 0;
    }
}

static operators get_operator(token_type_e type) {
    switch (type) {
        case TOKEN_ASSIGN_DIVIDE:
            return OP_DIVIDE;
        case TOKEN_ASSIGN_MULTIPLY:
            return OP_MULTIPLY;
        case TOKEN_ASSIGN_PLUS:
            return OP_ADD;
        case TOKEN_ASSIGN_MINUS:
            return OP_SUBTRACT;
        case TOKEN_OP_PLUS:
            return OP_ADD;
        case TOKEN_OP_MINUS:
            return OP_SUBTRACT;
        case TOKEN_OP_STAR:
            return OP_MULTIPLY;
        case TOKEN_OP_SLASH:
            return OP_DIVIDE;
        default:
            return OP_NONE;
    }
}

node_body* parse_body(parser_context* parser) {
    node_body* body = new_node_body(parser);
    body->loc = consume(parser, TOKEN_BRACE_OPEN).loc;

    while (!at(parser, TOKEN_BRACE_CLOSE)) {
        node* stmt = parse_statement(parser, 1);
        if(stmt != NULL) {
            list_push(parser, body->children, stmt);
        }
    }
    consume(parser, TOKEN_BRACE_CLOSE);

    return body;
}

node_literal* parse_literal(parser_context* parser) {
    node_literal* literal = new_node_literal(parser);
    literal->value = copy_string(parser, current(parser).data);
    literal->loc = current(parser).loc;
    step(parser, 1);
    return literal;
}

node_identifier* parse_identifier(parser_context* parser)  {
    node_identifier* identifier = new_node_identifier(parser);

    token_t name_token = consume(parser, TOKEN_ID);
    identifier->loc = name_token.loc;
    identifier->name = copy_string(parser, name_token.data);

    if(at(parser, TOKEN_PACKAGE_ACCESS)) {
        consume(parser, TOKEN_PACKAGE_ACCESS);
        // If we're at a package access we can't have any generics etc
        identifier->package = 1;
        identifier->child = parse_identifier(parser);

        return identifier;
    }

    // If it's not a package access, we first handle generics, then child identifiers.
    if(at(parser, TOKEN_GENERIC_OPEN)) {
        consume(parser, TOKEN_GENERIC_OPEN);
        while (1) {
            // We get the generic value
            node* generic = parse_statement(parser, 0);
            if(generic != NULL) {
                list_push(parser, identifier->generic_values, generic);
            }

            // If the next one isn't a comma it means we're done here.
            if(!at(parser, TOKEN_COMMA))
                break;

            consume(parser, TOKEN_COMMA);
        }
        // And then end it with the closing symbol.
        consume(parser, TOKEN_GENERIC_CLOSE);
    }

    // Now let's get our child if it exists.
    if(at(parser, TOKEN_ACCESS)) {
        consume(parser, TOKEN_ACCESS);
        identifier->child = parse_identifier(parser);
    }

    return identifier;
}

node_package_def* parse_package(parser_context* parser) {
    node_package_def* package = new_node_package_def(parser);
    package->loc = consume(parser, TOKEN_KW_PACKAGE).loc;

    token_t name_token = consume(parser, TOKEN_ID);
    package->package_name = copy_string(parser, name_token.data);

    return package;
}

node_use_def* parse_package_use(parser_context* parser) {
    node_use_def* package = new_node_use_def(parser);
    package->loc = consume(parser, TOKEN_KW_USE).loc;

    token_t name_token = consume(parser, TOKEN_ID);
    package->package_name = copy_string(parser, name_token.data);

    return package;
}

node* parse_variable_definition(parser_context* parser, permissions perms) {
    node_variable_def* variable_def = new_node_variable_def(parser);
    variable_def->flags = perms;

    // The structure is:
    // [modifiers] var [name] (-> [type]) (<- [statement]);
    // Modifiers is handled in the preceding step.
    variable_def->loc = consume(parser, TOKEN_KW_VAR).loc;
    variable_def->name = copy_string(parser, consume(parser, TOKEN_ID).data);

    if(at(parser, TOKEN_ACCESS)) {
        // We've got a type!
        consume(parser, TOKEN_ACCESS);

        variable_def->value_type = parse_identifier(parser);
    }

    if(at(parser, TOKEN_ASSIGN)) {
        // We've got a default type!
        consume(parser, TOKEN_ASSIGN);
        variable_def->default_value = parse_expression(parser);
    }

    consume(parser, TOKEN_END_STMT);

    return (node*)variable_def;
}

node* parse_function_definition(parser_context* parser, permissions perms) {
    node_function_def* function_def = new_node_function_def(parser);
    function_def->flags = perms;

    // Syntax:
    // [modifiers] function [name]([parameters]) (-> [return type])
    // Then either { [body] } or a semicolon, depending on the function declaration.
    // Modifiers are already handled.
    // No return type means it's a void.
    function_def->loc = consume(parser, TOKEN_KW_FUNCTION).loc;

    // [name]
    function_def->name = copy_string(parser, consume(parser, TOKEN_ID).data);

    // Parameters
    consume(parser, TOKEN_PARENTHESIS_OPEN);
    if(!at(parser, TOKEN_PARENTHESIS_CLOSE)) { // If we're not immediately at a close there's values.
        while (1) {

            // A parameter is
            // [name] [type] (*<- [default value])
            // Followed by a comma if there's multiple.

            node_parameter* parameter = new_node_parameter(parser);
            parameter->name = copy_string(parser, consume(parser, TOKEN_ID).data);
            parameter->value_type = parse_identifier(parser);

            if(at(parser, TOKEN_ASSIGN)) {
                consume(parser, TOKEN_ASSIGN);
                parameter->default_value = parse_expression(parser);
            }

            list_push(parser, function_def->parameters, (node*)parameter);

            if(!at(parser, TOKEN_COMMA))
                break;

            consume(parser, TOKEN_COMMA);
        }
    }
    consume(parser, TOKEN_PARENTHESIS_CLOSE);

    if(at(parser, TOKEN_ACCESS)) {
        consume(parser, TOKEN_ACCESS);
        function_def->return_type = parse_identifier(parser);
    }

    // Extern functions don't declare a body
    if(perms & PERMS_EXTERN) {
        consume(parser, TOKEN_END_STMT);
        return (node*)function_def;
    }

    // TODO: Function body parsing.
    function_def->body = parse_body(parser);

    return (node*)function_def;
}

node* parse_class_definition(parser_context* parser, permissions perms) {
    node_class_def* class_def = new_node_class_def(parser);
    class_def->flags = perms;

    // Structure:
    // [modifiers] class [name]{[generics]} { ... }

    class_def->loc = consume(parser, TOKEN_KW_CLASS).loc;

    class_def->name = copy_string(parser, consume(parser, TOKEN_ID).data);

    if(at(parser, TOKEN_GENERIC_OPEN)) {
        consume(parser, TOKEN_GENERIC_OPEN);

        while (1) {
            node_literal* literal = new_node_literal(parser);
            literal->value = copy_string(parser, consume(parser, TOKEN_ID).data);
            list_push(parser, class_def->generics, (node*)literal);

            if(!at(parser, TOKEN_COMMA))
                break;
            consume(parser, TOKEN_COMMA);
        }

        consume(parser, TOKEN_GENERIC_CLOSE);
    }

    // TODO: Class body

    class_def->body = parse_body(parser);

    return (node*)class_def;
}

node* parse_struct_definition(parser_context* parser, permissions perms) {
    node_struct_def* struct_def = new_node_struct_def(parser);
    struct_def->flags = perms;

    // Structs are like low-level classes.
    struct_def->loc = consume(parser, TOKEN_KW_STRUCT).loc;
    struct_def->name = copy_string(parser, consume(parser, TOKEN_ID).data);
    struct_def->body = parse_body(parser);

    return (node*)struct_def;
}

node* parse_definition(parser_context* parser) {
    permissions perms = PERMS_NONE;

    while (1) {
        // Basic permissions
        if(at(parser, TOKEN_KW_PRIVATE)) {
            consume(parser, TOKEN_KW_PRIVATE);
            perms |= PERMS_PRIVATE;
        } else if(at(parser, TOKEN_KW_SET)) {
            consume(parser, TOKEN_KW_SET);
            perms |= PERMS_PUBLIC;
        } else if(at(parser, TOKEN_KW_STATIC)) {
            consume(parser, TOKEN_KW_STATIC);
            perms |= PERMS_STATIC;
        } else if(at(parser, TOKEN_KW_EXTERN)) {
            consume(parser, TOKEN_KW_EXTERN);
            perms |= PERMS_EXTERN;
        } else if(at(parser, TOKEN_KW_UNSAFE)) {
            consume(parser, TOKEN_KW_UNSAFE);
            perms |= PERMS_UNSAFE;
        }

        // Definitions

        else if(at(parser, TOKEN_KW_VAR)) {
            return parse_variable_definition(parser, perms);
        } else if(at(parser, TOKEN_KW_FUNCTION)) {
            return parse_function_definition(parser, perms);
        } else if(at(parser, TOKEN_KW_CLASS)) {
            return parse_class_definition(parser, perms);
        } else if(at(parser, TOKEN_KW_STRUCT)) {
            return parse_struct_definition(parser, perms);
        }

        // Error
        else {
            throw_invalid_token(current(parser));
            break;
        }
    }
}

static int is_assign(token_type_e type) {
    return type == TOKEN_ASSIGN || type == TOKEN_ASSIGN_DIVIDE ||
    type == TOKEN_ASSIGN_MINUS || type == TOKEN_ASSIGN_MULTIPLY ||
    type == TOKEN_ASSIGN_PLUS;
}

static int is_single_op(token_type_e type) {
    return type == TOKEN_INCREMENT || type == TOKEN_DECREMENT;
}

static node_function_call* parse_function_call(parser_context* parser, node_identifier* identifier) {
    node_function_call* call = new_node_function_call(parser);

    call->target = identifier;
    call->loc = identifier->loc;

    consume(parser, TOKEN_PARENTHESIS_OPEN);

    if(!at(parser, TOKEN_PARENTHESIS_CLOSE)) {
        while (1) {
            node *value = parse_expression(parser);
            if (value != NULL)
                list_push(parser, call->parameters, value);
            if (!at(parser, TOKEN_COMMA))
                break;
            consume(parser, TOKEN_COMMA);
        }
    }

    consume(parser, TOKEN_PARENTHESIS_CLOSE);

    return call;
}

static node* parse_name_or_call(parser_context* parser) {
    node_identifier* identifier = parse_identifier(parser);

    if(at(parser, TOKEN_PARENTHESIS_OPEN)) {
        return (node*)parse_function_call(parser, identifier);
    }
    return (node*)identifier;

}

static node* parse_parenthesis_expression(parser_context* parser) {
    consume(parser, TOKEN_PARENTHESIS_OPEN);
    node* value = parse_expression(parser);
    consume(parser, TOKEN_PARENTHESIS_CLOSE);
    return value;
}

// Parser one of the sides in an expression.
static node* parse_primary_expression(parser_context* parser) {
    if(at(parser, TOKEN_STRING))
        return (node*)parse_literal(parser);
    if(at(parser, TOKEN_NUMERIC))
        return (node*)parse_literal(parser);

    if(at(parser, TOKEN_TRUE))
        return (node*)parse_literal(parser);
    if(at(parser, TOKEN_FALSE))
        return (node*)parse_literal(parser);

    if(at(parser, TOKEN_ID))
        return (node*)parse_name_or_call(parser);

    if(at(parser, TOKEN_PARENTHESIS_OPEN))
        return parse_parenthesis_expression(parser);

    throw_invalid_token(current(parser));
    return NULL;
}

// Thanks to the rgoc project.
// I have no clue how this works lol.
static node* parse_binary_expression(parser_context* parser, int parent_precedence) {
    node* left = NULL;

    int unary_precedence = unary_op_precedence(current(parser).type);
    if(unary_precedence != 0 && unary_precedence > parent_precedence) {
        token_type_e operator = consume(parser, current(parser).type).type;
        node* operand = parse_binary_expression(parser, unary_precedence);

        node_unary_exp* unary = new_node_unary_exp(parser);
        unary->operator = get_operator(operator);
        unary->operand = operand;
        return (node*)unary;
    } else {
        left = parse_primary_expression(parser);

        // TODO: Expanded primary expressions
        // ???? what did I mean with this?
    }

    // Time for magic
    while (1) {
        int precedence = binary_op_precedence(current(parser).type);

        if(precedence == 0 || precedence <= parent_precedence) {
            break;
        }

        operators operator = get_operator(current(parser).type);
        step(parser, 1);

        node* right = parse_binary_expression(parser, precedence);

        node_binary_exp* binary = new_node_binary_exp(parser);
        binary->operator = operator;
        binary->left = left;
        binary->right = right;

        left = (node*)binary;
    }

    return left;
}

static node_make* parse_make(parser_context* parser) {
    node_make* make = new_node_make(parser);
    make->loc = consume(parser, TOKEN_KW_MAKE).loc;

    make->target = parse_identifier(parser);

    consume(parser, TOKEN_PARENTHESIS_OPEN);
    if(!at(parser, TOKEN_PARENTHESIS_CLOSE)) {
        while (1) {
            node *value = parse_expression(parser);
            if (value != NULL)
                list_push(parser, make->parameters, value);
            if (!at(parser, TOKEN_COMMA))
                break;
            consume(parser, TOKEN_COMMA);
        }
    }
    consume(parser, TOKEN_PARENTHESIS_CLOSE);

    return make;
}

static node_return* parse_return(parser_context* parser) {
    node_return* return_stmt = new_node_return(parser);

    return_stmt->loc = consume(parser, TOKEN_KW_RETURN).loc;

    return_stmt->value = parse_expression(parser);

    return return_stmt;
}

static node_while* parse_while(parser_context* parser) {
    node_while* statement = new_node_while(parser);

    statement->loc = consume(parser, TOKEN_KW_WHILE).loc;

    consume(parser, TOKEN_PARENTHESIS_OPEN);
    statement->condition = parse_expression(parser);
    consume(parser, TOKEN_PARENTHESIS_CLOSE);

    if(at(parser, TOKEN_BRACE_OPEN)) {
        statement->execute = parse_body(parser);
    } else {
        // Make a body w/ a single line
        statement->execute = new_node_body(parser);
        list_push(parser, statement->execute->children, parse_statement(parser, 1));
    }

    return statement;
}

static node_if* parse_if(parser_context* parser) {
    node_if* statement = new_node_if(parser);

    // if([stmt]) { [...] } else if([stmt]) { [...] } else { [...] }

    statement->loc = consume(parser, TOKEN_KW_IF).loc;

    consume(parser, TOKEN_PARENTHESIS_OPEN);
    statement->condition = parse_expression(parser);
    consume(parser, TOKEN_PARENTHESIS_CLOSE);

    // We can either have multiple statements within braces or we just
    // have a single statement on the next line
    if(at(parser, TOKEN_BRACE_OPEN)) {
        statement->execute = parse_body(parser);
    } else {
        // Make a body w/ a single line
        statement->execute = new_node_body(parser);
        list_push(parser, statement->execute->children, parse_statement(parser, 1));
    }

    // There's another clause
    if(at(parser, TOKEN_KW_ELSE)) {
        consume(parser, TOKEN_KW_ELSE);

        // else if statement
        if(at(parser, TOKEN_KW_IF)) {
            statement->else_case = (node*)parse_if(parser);
        } else {

            // We once again check for a multi-line case.
            if(at(parser, TOKEN_BRACE_OPEN)) {
                statement->else_case = (node*)parse_body(parser);
            } else {
                // Make a body w/ a single line
                statement->else_case = (node*)parse_statement(parser, 1);
            }
        }
    }

    return statement;

}

static node* parse_expression(parser_context* parser) {
    int parser_old_location = parser->token_current;

    if(at(parser, TOKEN_ID)) {
        // We're doing something with accessors
        node_identifier* id = parse_identifier(parser);

        // If we're trying to assign something
        if(is_assign(current(parser).type)) {
            operators operator = OP_NONE;
            step(parser, 1);
            node_assignation* assignation = new_node_assignation(parser);
            assignation->operator = operator;
            assignation->target = id;
            assignation->value = parse_expression(parser);

            return (node*)assignation;
        }
        if(is_single_op(current(parser).type)) {
            // i++ is the same as i <-+ 1
            // This means we can just turn it into an assignation.

            node_literal* literal = new_node_literal(parser);
            literal->value = copy_string(parser, "1");

            operators operator = OP_NONE;
            switch (current(parser).type) {
                case TOKEN_INCREMENT:
                    operator = OP_ADD;
                    break;
                case TOKEN_DECREMENT:
                    operator = OP_SUBTRACT;
                    break;
                default:
                    break;
            }

            step(parser, 1);
            node_assignation* assignation = new_node_assignation(parser);
            assignation->operator = operator;
            assignation->target = id;
            assignation->value = (node*)literal;
            return (node*)assignation;
        }
    }

    if(at(parser, TOKEN_KW_MAKE)) {
        return (node*)parse_make(parser);
    }

    parser->token_current = parser_old_location;

    return parse_binary_expression(parser, 0);
}

// Return and optionally consume a semicolon.
// _semi is if this specific parse-ing should consume when needed.
// _value is the value to return.
#define PARSE(_semi, _value) { node* n = (node*)_value; if(semicolon && _semi) consume(parser, TOKEN_END_STMT); return n; }

// This is the main part of the parser.
// semicolons may be enforced when needed, but only for statements that need a semicolon.
node* parse_statement(parser_context* parser, int semicolon) {

    // Package-related statements
    if(at(parser, TOKEN_KW_PACKAGE)) PARSE(1, parse_package(parser))
    if(at(parser, TOKEN_KW_USE)) PARSE(1, parse_package_use(parser))

    // Definitions - They handle their own damn semicolons
    if(at(parser, TOKEN_KW_PRIVATE)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_SET)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_STATIC)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_EXTERN)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_UNSAFE)) PARSE(0, parse_definition(parser))

    if(at(parser, TOKEN_KW_VAR)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_FUNCTION)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_CLASS)) PARSE(0, parse_definition(parser))
    if(at(parser, TOKEN_KW_STRUCT)) PARSE(0, parse_definition(parser))

    // Return
    if(at(parser, TOKEN_KW_RETURN)) PARSE(1, parse_return(parser))

    // Control flow statements
    if(at(parser, TOKEN_KW_IF)) PARSE(0, parse_if(parser))
    if(at(parser, TOKEN_KW_WHILE)) PARSE(0, parse_while(parser))
    // TODO: for loops

    // Finally, we do expressions.
    PARSE(1, parse_expression(parser))
}


void parser_parse(parser_context* parser) {
    node_root* root_node = new_node_root(parser);
    parser->node = root_node;

    while(current(parser).type != TOKEN_EOF) {
        node* n = parse_statement(parser, 1);
        if(n != NULL)
            list_push(parser, root_node->children, n);
    }
}