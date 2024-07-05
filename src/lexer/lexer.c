#include "lexer.h"
#include <stdlib.h>
#include "token.h"
#include <string.h>
#include "../errors/error.h"

static int streq(char* a, char* b) {
    int a_len = strlen(a);
    int b_len = strlen(b);
    if(a_len != b_len)
        return 0;
    for (int i = 0; i < a_len; ++i) {
        if(a[i] != b[i])
            return 0;
    }
    return 1;
}

static int peek(lexer_context* context, int amt) {
    if(context->position + amt >= context->data_length)
        return EOF;
    return context->data[context->position + amt];
}

static int current(lexer_context* context) {
    return peek(context, 0);
}

static void step(lexer_context* context, int amt) {
    for (int i = 0; i < amt; ++i) {
        if(current(context) == EOF)
            break;
        if(current(context) == '\n') {
            context->loc.column = 1;
            context->loc.line++;
        } else if(current(context) == '\t') {
            // TODO: Decide if this is dumb.
            context->loc.column += 4;
        } else {
            context->loc.column++;
        }
        context->position++;
    }
}

static int consume(lexer_context* context) {
    int c = current(context);
    step(context, 1);
    return c;
}

// Push a token to the lexer context array
static void push_token(lexer_context* lexer, token_t token) {
    if(lexer->tokens_allocated <= lexer->token_count + 1) {
        lexer->tokens = reallocarray(lexer->tokens, lexer->tokens_allocated * 2 + 1, sizeof(token));
        lexer->tokens_allocated = lexer->tokens_allocated * 2 + 1;
    }
    lexer->tokens[lexer->token_count] = token;
    lexer->token_count++;
}

// Push a token with only a type
static void push_empty_token(lexer_context* lexer, token_type_e type) {
    token_t token;
    token.type = type;
    token.data = NULL;
    token.loc = lexer->loc;
    push_token(lexer, token);
}

// Initialize a lexer context
lexer_context* lexer_create() {
    lexer_context* lexer = (lexer_context*)malloc(sizeof(lexer_context));
    lexer->token_count = 0;
    lexer->tokens_allocated = 0;
    lexer->tokens = NULL;
    lexer->loc = (location){1, 1};
    return lexer;
}

// Delete a lexer context
void lexer_destroy(lexer_context* context) {
    for (int i = 0; i < context->token_count; ++i) {
        free(context->tokens[i].data);
    }

    free(context);
}

static int look_ahead_equals(lexer_context* context, const char* str) {
    unsigned long length = strlen(str);
    for (int i = 0; i < length; ++i) {
        int at = peek(context, i);
        if(at != str[i])
            return 0;
    }
    return 1;
}

static int is_char_numeric(int c) {
    // Presuming we use ASCII here...
    return c >= '0' && c <= '9';
}

// We allow A-Z in any capitalization plus underscores in the start of a word/identifier
static int is_char_word_starter(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '@';
}
// Then 2nd character onward allows us to do numbers tool.
static int is_char_word(int c) {
    return is_char_word_starter(c) || is_char_numeric(c);
}

// This monster is responsible for tokenizing strings.
static void lex_string(lexer_context* context) {
    // Average sting would be max 32 chars I'd guess
    char* read_buffer = malloc(32);
    // Due to performance concerns, we'll be using squared list growth(or whatever it is called).
    // Since read_buffer doesn't grow every iteration, we need to keep track of the allocated size.
    int length = 0;
    int buffer_size = 32;
    token_t token;
    token.loc = context->loc;
    token.type = TOKEN_STRING;

    step(context, 1);

    while (1) {
        int c = current(context);
        if(c == EOF) {
            free(read_buffer);
            error_throw("RCT1011", context->loc, "Unexpected EOF");
            return;
        }
        if(c == '\n') {
            free(read_buffer);
            error_throw("RCT1011", context->loc, "Unexpected newline");
            return;
        }
        if(c == '"') break;

        // All right, time to add on this character.
        // First we make sure there's enough space for us.
        if(buffer_size <= length + 1) {
            // If there isn't, reallocate the buffer with double size.
            char* new = realloc(read_buffer, buffer_size * 2);
            if(new == NULL) {
                // Ff we can't reallocate we just throw an error and quit.
                fprintf(stderr, "String read buffer could not be reallocated\n");
                free(read_buffer);
                exit(EXIT_FAILURE);
            }
            // Otherwise we set our read buffer to the resized one and update our variables.
            read_buffer = new;
            buffer_size *= 2;
        }
        // Then we can add our character
        read_buffer[length] = (char)c;
        length++;
        step(context, 1);
    }

    step(context, 1);

    // We might not need this resizing pass, since our buffer might already
    // be big enough, but just to be safe. I'll have to take a look at this :)
    // (preferably I'd like to cut this piece out since it's duplicate code)
    if(buffer_size <= length + 1) {
        char* new = realloc(read_buffer, buffer_size * 2);
        if(new == NULL) {
            fprintf(stderr, "String read buffer could not be reallocated\n");
            free(read_buffer);
            exit(EXIT_FAILURE);
        }
        read_buffer = new;
    }
    read_buffer[length] = 0;

    // Then we build our token and push it.
    // DON'T FREE THE READ BUFFER!!!
    token.data = read_buffer;

    push_token(context, token);
}

#define KW_CHECK(_s, _id) if(streq(token.data, _s)) token.type = _id;

// We treat ourselves to duplicate code for word tokenization.
static void lex_word(lexer_context* context) {
    // Average word would be 32 chars I'd guess
    char* read_buffer = malloc(32);
    // Due to performance concerns, we'll be using squared list growth(or whatever it is called).
    // Since read_buffer doesn't grow every iteration, we need to keep track of the allocated
    // size.
    int length = 0;
    int buffer_size = 32;

    token_t token;
    token.loc = context->loc;

    int c;
    while (1) {
        c = current(context);
        if(!is_char_word(c)) break;

        // All right, time to add on this character.
        // First we make sure there's enough space for us.
        if(buffer_size <= length + 1) {
            // If there isn't, reallocate the buffer with double size.
            char* new = realloc(read_buffer, buffer_size * 2);
            if(new == NULL) {
                // Ff we can't reallocate we just throw an error and quit.
                fprintf(stderr, "Word read buffer could not be reallocated\n");
                free(read_buffer);
                exit(EXIT_FAILURE);
            }
            // Otherwise we set our read buffer to the resized one and update our variables.
            read_buffer = new;
            buffer_size *= 2;
        }
        // Then we can add our character
        read_buffer[length] = (char)c;
        length++;
        step(context, 1);
    }


    // We might not need this resizing pass, since our buffer might already
    // be big enough, but just to be safe. I'll have to take a look at this :)
    // (preferably I'd like to cut this piece out since it's duplicate code)
    if(buffer_size <= length + 1) {
        char* new = realloc(read_buffer, buffer_size * 2);
        if(new == NULL) {
            fprintf(stderr, "Word read buffer could not be reallocated\n");
            free(read_buffer);
            exit(EXIT_FAILURE);
        }
        read_buffer = new;
    }
    read_buffer[length] = 0;

    // Then we build our token and push it.
    // DON'T FREE THE READ BUFFER!!!
    token.data = read_buffer;

    // Time to do some string checking ig
    KW_CHECK("var", TOKEN_KW_VAR)
    else KW_CHECK("private", TOKEN_KW_PRIVATE)
    else KW_CHECK("set", TOKEN_KW_SET)
    else KW_CHECK("extern", TOKEN_KW_EXTERN)
    else KW_CHECK("static", TOKEN_KW_STATIC)
    else KW_CHECK("unsafe", TOKEN_KW_UNSAFE)
    else KW_CHECK("if", TOKEN_KW_IF)
    else KW_CHECK("else", TOKEN_KW_ELSE)
    else KW_CHECK("for", TOKEN_KW_FOR)
    else KW_CHECK("function", TOKEN_KW_FUNCTION)
    else KW_CHECK("return", TOKEN_KW_RETURN)
    else KW_CHECK("continue", TOKEN_KW_CONTINUE)
    else KW_CHECK("break", TOKEN_KW_BREAK)
    else KW_CHECK("make", TOKEN_KW_MAKE)
    else KW_CHECK("class", TOKEN_KW_CLASS)
    else KW_CHECK("struct", TOKEN_KW_STRUCT)
    else KW_CHECK("enum", TOKEN_KW_ENUM)
    else KW_CHECK("package", TOKEN_KW_PACKAGE)
    else KW_CHECK("use", TOKEN_KW_USE)
    else KW_CHECK("alias", TOKEN_KW_ALIAS)
    else KW_CHECK("while", TOKEN_KW_WHILE)
    //else KW_CHECK("ref", TOKEN_KW_REF)

#ifdef NRC_JOKES
#warning Jokes are enabled. They are not part of the spec
    // And the joke-y ones
    else KW_CHECK("mine", TOKEN_KW_PRIVATE)
#endif

    else token.type = TOKEN_ID;

    push_token(context, token);
}

// Imagine if we had a template for this and not just ctrl+c ctrl+v
// See comments in lex_string and lex_word where missing!

static void lex_numeric(lexer_context* context) {
    // Average numeric would be 8 chars I'd guess, instead of 32.
    char* read_buffer = malloc(8);
    int length = 0;
    int buffer_size = 8;

    token_t token;
    token.loc = context->loc;

    int c;
    int has_decimal = 0;
    while (1) {
        c = current(context);

        // TODO: Decimals fucking suck.

        // We allow underscore for splitting apart long numerics like 1000000 into 1_000_000
        if(c == '_') { step(context, 1); continue;}
        if(c == '.' && !has_decimal) {
            has_decimal = 1;
        }
        else if(!is_char_numeric(c)) break;
        if(buffer_size <= length + 1) {
            char* new = realloc(read_buffer, buffer_size * 2);
            if(new == NULL) {
                fprintf(stderr, "Numeric read buffer could not be reallocated\n");
                free(read_buffer);
                exit(EXIT_FAILURE);
            }

            read_buffer = new;
            buffer_size *= 2;
        }

        read_buffer[length] = (char)c;
        length++;
        step(context, 1);
    }


    if(buffer_size <= length + 2) {
        char* new = realloc(read_buffer, buffer_size * 2);
        if(new == NULL) {
            fprintf(stderr, "Numeric read buffer could not be reallocated\n");
            free(read_buffer);
            exit(EXIT_FAILURE);
        }
        read_buffer = new;
    }
    if(current(context) == 'f') {
        read_buffer[length] = 'f';
        step(context, 1);
        length++;
    }
    read_buffer[length] = 0;

    // Then we build our token and push it.
    // DON'T FREE THE READ BUFFER!!!
    token.data = read_buffer;
    token.type = TOKEN_NUMERIC;

    push_token(context, token);
}

void lex_preprocessor(lexer_context* context) {
    // For now, we read 'til the end.
    int did_break = 0;
    while (1) {
        step(context, 1);
        if(current(context) == '\\')
            did_break = 1;
        else if(current(context) == '\n' && !did_break)
            break;
        else if(current(context) == EOF)
            break;
        else
            did_break = 0;
    }
}

void lex_comment_big(lexer_context* context) {
    step(context, 2);
    while (current(context) != '*' || peek(context, 1) != '/') {

        if(current(context) == EOF) return;

        if(current(context) == '/' && peek(context, 1) == '*') lex_comment_big(context);
        step(context, 1);
    }
    step(context, 2);
}

// Characters we don't care about, such as whitespace
#define CASE_SKIP(_char) if(current(context)  == _char) { step(context, 1); continue; }
// Single-character tokens
#define CASE_SIMPLE(_char, _type) if(current(context) == _char) { step(context, 1); push_empty_token(context, _type); continue; }
// Multi-character tokens WITHOUT any attached data
#define CASE_SIMPLE_MULT(_str, _type) { if(look_ahead_equals(context, _str)) { step(context, sizeof(_str) - 1); push_empty_token(context, _type); continue; } }

void lexer_process(lexer_context* context) {
    while(current(context) != EOF) {
        // Time to iterate through a million different cases.

        if(current(context) == '/' && peek(context, 1) == '/') {
            while (current(context) != '\n' && current(context) != EOF) step(context, 1);
            continue;
        }

        if(current(context) == '#') {
            lex_preprocessor(context);
            continue;
        }

        if(current(context) == '/' && peek(context, 1) == '*') {
            lex_comment_big(context);
            continue;
        }

        CASE_SKIP(' ')
        CASE_SKIP('\t')
        CASE_SKIP('\n')
        CASE_SKIP('\r')
        CASE_SIMPLE(';', TOKEN_END_STMT)

        CASE_SIMPLE('(', TOKEN_PARENTHESIS_OPEN)
        CASE_SIMPLE(')', TOKEN_PARENTHESIS_CLOSE)

        CASE_SIMPLE('{', TOKEN_BRACE_OPEN)
        CASE_SIMPLE('}', TOKEN_BRACE_CLOSE)

        CASE_SIMPLE('[', TOKEN_BRACKET_OPEN)
        CASE_SIMPLE(']', TOKEN_BRACKET_CLOSE)

        CASE_SIMPLE(',', TOKEN_COMMA)

        CASE_SIMPLE_MULT("::", TOKEN_PACKAGE_ACCESS)

        // Characters like minus and "<" need to be checked in order of size.
        // If we didn't, we might get [TOKEN_OP_LT, TOKEN_OP_MINUS] when we meant [TOKEN_ASSIGN].

        CASE_SIMPLE_MULT("->", TOKEN_ACCESS)

        CASE_SIMPLE_MULT("<-", TOKEN_ASSIGN)
        CASE_SIMPLE_MULT("<-/", TOKEN_ASSIGN_DIVIDE)
        CASE_SIMPLE_MULT("<-*", TOKEN_ASSIGN_MULTIPLY)
        CASE_SIMPLE_MULT("<--", TOKEN_ASSIGN_MINUS)
        CASE_SIMPLE_MULT("<-+", TOKEN_ASSIGN_PLUS)

        CASE_SIMPLE_MULT("<=", TOKEN_OP_LT_EQUALS)
        CASE_SIMPLE_MULT(">=", TOKEN_OP_GR_EQUALS)

        CASE_SIMPLE_MULT("++", TOKEN_INCREMENT)
        CASE_SIMPLE_MULT("--", TOKEN_DECREMENT)

        CASE_SIMPLE('=', TOKEN_OP_EQUALS)

        CASE_SIMPLE('<', TOKEN_OP_LT)
        CASE_SIMPLE('>', TOKEN_OP_GT)

        CASE_SIMPLE('+', TOKEN_OP_PLUS)
        CASE_SIMPLE('-', TOKEN_OP_MINUS)
        CASE_SIMPLE('*', TOKEN_OP_STAR)
        CASE_SIMPLE('/', TOKEN_OP_SLASH)

        if(current(context) == '"') { lex_string(context); continue; }
        if(is_char_word_starter(current(context))) { lex_word(context); continue; }
        if(is_char_numeric(current(context))) { lex_numeric(context); continue; }
        if(current(context) == '.') { lex_numeric(context); continue; }
        // For now, we just get mad :)
        step(context, 1);
        error_throw("RCT1010", context->loc, "Invalid character %2X(%c)", current(context), current(context));
    }
    push_empty_token(context, TOKEN_EOF);
}

void lexer_read(lexer_context* lexer, FILE* file) {
    long pos = ftell(file);
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    fseek(file, pos, SEEK_SET);

    void* new_buffer = realloc(lexer->data, lexer->data_length + size);
    if(new_buffer == NULL)
        return;

    lexer->data = new_buffer;
    fread(lexer->data + lexer->data_length, sizeof(char), size, file);
    lexer->data_length += size;
}

void lexer_push(lexer_context* lexer, char* data, int length) {

    void* new_buffer = realloc(lexer->data, lexer->data_length + length);
    if(new_buffer == NULL)
        return;

    lexer->data = new_buffer;
    memcpy(lexer->data + lexer->data_length, data, length);
    lexer->data_length += length;
}