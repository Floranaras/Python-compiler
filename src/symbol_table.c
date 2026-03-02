#include "utils.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_CAP 64

/* Release heap memory owned by a value (strings only). */
static void value_release(struct value *v)
{
	if (!v)
		return;
	if (v->type == VALUE_STRING)
		free(v->data.string);
}

/**
 * symbol_table_create() - Allocate a new symbol table scope.
 */
struct symbol_table *symbol_table_create(struct symbol_table *parent)
{
	struct symbol_table *table;

	table = calloc(1, sizeof(*table));
	if (!table)
		goto err_table;

	table->symbols = malloc(sizeof(struct symbol) * INIT_CAP);
	if (!table->symbols)
		goto err_symbols;

	table->count = 0;
	table->capacity = INIT_CAP;
	table->parent = parent;
	return table;

err_symbols:
	free(table);
err_table:
	fprintf(stderr, "symbol_table: out of memory\n");
	return NULL;
}

/**
 * symbol_table_destroy() - Free a scope and all of its bindings.
 */
void symbol_table_destroy(struct symbol_table *table)
{
	int j;

	if (!table)
		return;

	for (j = 0; j < table->count; j++) {
		free(table->symbols[j].name);
		value_release(&table->symbols[j].value);
	}

	free(table->symbols);
	free(table);
}

/**
 * symbol_table_find() - Walk the scope chain to find a binding.
 */
struct symbol *symbol_table_find(struct symbol_table *table,
				 const char *name)
{
	int j;

	if (!table || !name)
		return NULL;

	while (table) {
		for (j = 0; j < table->count; j++) {
			if (!strcmp(table->symbols[j].name, name))
				return &table->symbols[j];
		}
		table = table->parent;
	}

	return NULL;
}

/* grow_symbols() - Double the symbol array capacity. */
static int grow_symbols(struct symbol_table *table)
{
	struct symbol *new_syms;
	int new_cap;

	new_cap = table->capacity * 2;
	new_syms = realloc(table->symbols,
			   sizeof(struct symbol) * new_cap);
	if (!new_syms) {
		fprintf(stderr, "symbol_table: out of memory\n");
		return 0;
	}

	table->symbols  = new_syms;
	table->capacity = new_cap;
	return 1;
}

/**
 * symbol_table_set_local() - Bind a name in the current scope only.
 */
void symbol_table_set_local(struct symbol_table *table,
			    const char *name,
			    struct value value)
{
	int j;

	if (!table || !name)
		return;

	/* Update in place if the name already exists here. */
	for (j = 0; j < table->count; j++) {
		if (strcmp(table->symbols[j].name, name) != 0)
			continue;
		value_release(&table->symbols[j].value);
		table->symbols[j].value = value;
		return;
	}

	/* Grow if needed. */
	if (table->count >= table->capacity) {
		if (!grow_symbols(table))
			return;
	}

	table->symbols[table->count].name = strdup(name);
	if (!table->symbols[table->count].name) {
		fprintf(stderr, "symbol_table: out of memory\n");
		return;
	}

	table->symbols[table->count].value = value;
	table->count++;
}

/**
 * symbol_table_set() - Assign respecting the full scope chain.
 */
void symbol_table_set(struct symbol_table *table,
		      const char *name,
		      struct value value)
{
	struct symbol *existing;

	if (!table || !name)
		return;

	existing = symbol_table_find(table, name);
	if (existing) {
		value_release(&existing->value);
		existing->value = value;
		return;
	}

	symbol_table_set_local(table, name, value);
}
