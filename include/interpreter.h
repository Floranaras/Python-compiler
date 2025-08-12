#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "symbol_table.h"
#include "ast.h"

// =============================================================================
// INTERPRETER/EVALUATOR
// =============================================================================

typedef struct {
    SymbolTable* global_scope;
    SymbolTable* current_scope;
    Value return_value;
    bool has_returned;
} Interpreter;

// Create interpreter
Interpreter* interpreter_create();

// Main evaluation function
Value interpreter_evaluate(Interpreter* interp, ASTNode* node);

#endif // INTERPRETER_H