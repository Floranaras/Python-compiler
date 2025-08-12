#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

// Create new symbol table
SymbolTable* symbol_table_create(SymbolTable* parent) {
    SymbolTable* table = malloc(sizeof(SymbolTable));
    table->symbols = malloc(sizeof(Symbol) * 64);
    table->count = 0;
    table->capacity = 64;
    table->parent = parent;
    return table;
}

// Find symbol in current table or parent tables
Symbol* symbol_table_find(SymbolTable* table, char* name) {
    // Search current table
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }

    // Search parent table if exists
    if (table->parent != NULL) {
        return symbol_table_find(table->parent, name);
    }

    return NULL; // Not found
}

// Set symbol value (create if doesn't exist)
void symbol_table_set(SymbolTable* table, char* name, Value value) {
    // Check if symbol already exists
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            table->symbols[i].value = value;
            return;
        }
    }

    // Create new symbol
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        table->symbols = realloc(table->symbols, sizeof(Symbol) * table->capacity);
    }

    table->symbols[table->count].name = malloc(strlen(name) + 1);
    strcpy(table->symbols[table->count].name, name);
    table->symbols[table->count].value = value;
    table->count++;
}