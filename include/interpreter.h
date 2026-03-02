#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "symbol_table.h"
#include "ast.h"

/*
 * Hard limit on call-stack depth.
 *
 * Each Python function call adds a C stack frame through
 * eval_function_call() -> interpreter_evaluate().  200 gives
 * comfortable headroom below the typical 8 MiB Linux thread stack.
 */
#define MAX_CALL_DEPTH	200

/**
 * struct interpreter - All mutable state for one execution run.
 * @global_scope:  Module-level symbol table.
 * @current_scope: Innermost active scope; equals @global_scope at
 *                 the top level.
 * @return_value:  Holds the pending return value while a call unwinds.
 * @has_returned:  Non-zero once a return statement has executed.
 * @call_depth:    Current call-stack depth; guarded by MAX_CALL_DEPTH.
 */
struct interpreter {
	struct symbol_table	*global_scope;
	struct symbol_table	*current_scope;
	struct value		 return_value;
	int			 has_returned;
	int			 call_depth;
};

/**
 * interpreter_create() - Allocate and initialise a new interpreter.
 *
 * Return: Pointer to interpreter, or NULL on allocation failure.
 */
struct interpreter *interpreter_create(void);

/**
 * interpreter_destroy() - Free an interpreter and its global scope.
 * @interp: Interpreter to destroy.  Safe to call with NULL.
 */
void interpreter_destroy(struct interpreter *interp);

/**
 * interpreter_evaluate() - Recursively evaluate an AST node.
 * @interp: Active interpreter state.
 * @node:   Node to evaluate.
 *
 * Return: Runtime value produced by this node.  Returns a VALUE_NONE
 *         value for statements and on error.
 */
struct value interpreter_evaluate(struct interpreter *interp,
				  struct ast_node *node);

#endif 
