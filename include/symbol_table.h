#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

/**
 * enum value_type - Runtime value discriminator.
 */
enum value_type {
	VALUE_NUMBER,		/* IEEE-754 double              */
	VALUE_STRING,		/* heap-allocated C string      */
	VALUE_FUNCTION,		/* borrowed pointer into AST    */
	VALUE_NONE		/* Python None / void           */
};

/**
 * struct value - A dynamically-typed runtime value.
 * @type: Which variant is active.
 * @data: Variant payload.
 *
 * VALUE_STRING owns its string; the holder is responsible for
 * freeing it when the value is overwritten or goes out of scope.
 * VALUE_FUNCTION is a borrowed pointer into the AST; it is never
 * freed through this struct.
 */
struct value {
	enum value_type type;
	union {
		double number;
		char *string;
		struct ast_node *function; /* points into AST; not owned */
	} data;
};

/**
 * struct symbol - A name-to-value binding.
 * @name:  Heap-allocated identifier string (owned).
 * @value: The bound runtime value.
 */
struct symbol {
	char		*name;
	struct value	 value;
};

/**
 * struct symbol_table - A single lexical scope.
 * @symbols:  Dynamic array of bindings.
 * @count:    Number of active bindings.
 * @capacity: Allocated capacity of @symbols.
 * @parent:   Enclosing scope, NULL for the global scope.
 */
struct symbol_table {
	struct symbol		*symbols;
	int			 count;
	int			 capacity;
	struct symbol_table	*parent;
};

/**
 * symbol_table_create() - Allocate a new symbol table scope.
 * @parent: Enclosing scope, or NULL for the global scope.
 *
 * Return: Pointer to the new table, or NULL on allocation failure.
 */
struct symbol_table *symbol_table_create(struct symbol_table *parent);

/**
 * symbol_table_destroy() - Free a scope and all of its bindings.
 * @table: Table to destroy.  Safe to call with NULL.
 *
 * Does not walk to parent scopes.
 */
void symbol_table_destroy(struct symbol_table *table);

/**
 * symbol_table_find() - Look up a name in this scope or any ancestor.
 * @table: Innermost scope to start the search.
 * @name:  Identifier to look up.
 *
 * Return: Pointer to the symbol if found, NULL otherwise.
 */
struct symbol *symbol_table_find(struct symbol_table *table,
				 const char *name);

/**
 * symbol_table_set_local() - Bind a name in the current scope only.
 * @table: Target scope.
 * @name:  Identifier (copied if creating a new binding).
 * @value: Value to bind.
 *
 * If @name already exists in @table it is updated in place.
 * Parent scopes are never modified.
 */
void symbol_table_set_local(struct symbol_table *table,
			     const char *name,
			     struct value value);

/**
 * symbol_table_set() - Assign respecting the full scope chain.
 * @table: Innermost scope.
 * @name:  Identifier.
 * @value: Value to bind.
 *
 * If @name exists anywhere in the scope chain it is updated there.
 * Otherwise a new binding is created in @table.  This matches
 * Python's default assignment semantics for module-level names.
 */
void symbol_table_set(struct symbol_table *table,
		      const char *name,
		      struct value value);

#endif /* SYMBOL_TABLE_H */
