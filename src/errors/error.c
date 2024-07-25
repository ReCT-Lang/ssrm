#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct error_list_node {
    error error;
    struct error_list_node* next;
} error_list_node;

error_list_node* root_node = NULL;
error_list_node* last = NULL;

static int print_errors = 0;

int enumerate_errors(error* list) {
    int count = 0;
    error_list_node* current_node = root_node;
    while(current_node != NULL) {
        if(list != NULL) {
            error e = {};
            e.loc = current_node->error.loc;
            e.code = current_node->error.code;
            e.string = (char*)calloc(sizeof(char), strlen(current_node->error.string) + 1);
            strcpy(e.string, current_node->error.string);
            list[count] = e;
        }

        current_node = current_node->next;
        count++;
    }
    return count;
}

void free_error(error err) {
    free(err.string);
}

void dispose_errors() {
    error_list_node* current_node = root_node;
    while(current_node != NULL) {
        free_error(current_node->error);
        current_node = current_node->next;
    }
    root_node = NULL;
    current_node = NULL;
}

void error_throw(error_code code, location loc, const char* fmt, ...) {
    va_list args;
    char* error_msg = malloc(2048);

    va_start(args, fmt);
    vsprintf(error_msg, fmt, args);
    va_end(args);

    error e = {.code = code, .loc = loc, .string = error_msg};

    if(print_errors)
        print_error(stderr, e);

    // We push everything to a linked list for ease.
    if(root_node == NULL) {
        root_node = malloc(sizeof(error_list_node));
        root_node->next = NULL;
        root_node->error = e;
        last = root_node;
    } else {
        last->next = malloc(sizeof(error_list_node));
        last->next->next = NULL;
        last->next->error = e;

        last = last->next;
    }
}

void allow_error_print(int enabled) {
    print_errors = enabled ? 1 : 0;
}

void print_error(FILE* file, error e) {
    fprintf(file, "[ERR] [%s: L: %u, C: %u] %s: %s\n", e.loc.context->path, e.loc.line, e.loc.column, e.code, e.string);
}