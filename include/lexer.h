#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include "token.h"

// =============================================================================
// LEXER (TOKENIZER)
// =============================================================================

typedef struct {
    char* source;       // Source code string
    int position;       // Current position in source
    int line;           // Current line number
    int column;         // Current column number
    int* indent_stack;  // Stack for tracking indentation levels
    int indent_top;     // Top of indent stack
    int indent_capacity;// Capacity of indent stack
    bool at_line_start; // Are we at the beginning of a line?
    int pending_dedents; // Number of DEDENT tokens waiting to be returned
} Lexer;

// Initialize the lexer with source code
Lexer* lexer_create(char* source);

// Free lexer memory
void lexer_destroy(Lexer* lexer);

// Get the next token from the source
Token lexer_next_token(Lexer* lexer);

#endif // LEXER_H
