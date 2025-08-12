#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"

// =============================================================================
// PARSER
// =============================================================================

typedef struct {
    Token* tokens;      // Array of tokens
    int position;       // Current position in token array
    int token_count;    // Total number of tokens
} Parser;

// Create parser with token array
Parser* parser_create(Token* tokens, int count);

// Free parser
void parser_destroy(Parser* parser);

// Parse entire program
ASTNode* parser_parse_program(Parser* parser);

#endif // PARSER_H