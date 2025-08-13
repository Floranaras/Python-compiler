#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

// =============================================================================
// SYMBOL TABLE (Variable and Function Storage)
// =============================================================================

typedef enum 
{
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_FUNCTION,
    VALUE_NONE
} ValueType;

typedef struct Value 
{
    ValueType type;
    union {
        double number;
        char* string;
        ASTNode* function;  // Points to function definition
    } data;
} Value;

typedef struct Symbol 
{
    char* name;
    Value value;
} Symbol;

typedef struct SymbolTable 
{
    Symbol* symbols;
    int count;
    int capacity;
    struct SymbolTable* parent;  // For nested scopes
} SymbolTable;

// Create new symbol table
SymbolTable* symbol_table_create(SymbolTable* parent);

// Destroy symbol table and free memory
void symbol_table_destroy(SymbolTable* table);

// Find symbol in current table or parent tables
Symbol* symbol_table_find(SymbolTable* table, char* name);

// Set symbol value (create if doesn't exist)
void symbol_table_set(SymbolTable* table, char* name, Value value);

#endif // SYMBOL_TABLE_H
