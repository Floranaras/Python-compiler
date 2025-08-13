#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

// Create new symbol table
SymbolTable* symbol_table_create(SymbolTable* parent) 
{
    SymbolTable* table = malloc(sizeof(SymbolTable));
    table->symbols = malloc(sizeof(Symbol) * 64);
    table->count = 0;
    table->capacity = 64;
    table->parent = parent;
    return table;
}

// Destroy symbol table and free memory
void symbol_table_destroy(SymbolTable* table) 
{
    if (table == NULL) return;
    
    // Free all symbol names and string values
    for (int i = 0; i < table->count; i++) {
        free(table->symbols[i].name);
        
        // Free string values if they exist
        if (table->symbols[i].value.type == VALUE_STRING) {
            free(table->symbols[i].value.data.string);
        }
    }
    
    free(table->symbols);
    free(table);
}

// Find symbol in current table or parent tables
Symbol* symbol_table_find(SymbolTable* table, char* name) 
{
    // Search current table
    for (int j = 0; j < table->count; j++) 
	{
        if (!strcmp(table->symbols[j].name, name))
            return &table->symbols[j];
    }

    // Search parent table if exists
    if (table->parent != NULL) 
        return symbol_table_find(table->parent, name);

    return NULL; // Not found
}

// Set symbol value (create if doesn't exist)
void symbol_table_set(SymbolTable* table, char* name, Value value) 
{
    // Check if symbol already exists
    for (int j = 0; j < table->count; j++) 
	{
        if (!strcmp(table->symbols[j].name, name)) 
		{
            // Free old string value if it exists
            if (table->symbols[j].value.type == VALUE_STRING) {
                free(table->symbols[j].value.data.string);
            }
            table->symbols[j].value = value;
            return;
        }
    }

    // Create new symbol
    if (table->count >= table->capacity) 
	{
        table->capacity *= 2;
        table->symbols = realloc(table->symbols, sizeof(Symbol) * table->capacity);
    }

    table->symbols[table->count].name = malloc(strlen(name) + 1);
    strcpy(table->symbols[table->count].name, name);
    table->symbols[table->count].value = value;
    table->count++;
}
