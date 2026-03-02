#include "utils.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 64

struct value interpreter_evaluate(struct interpreter *interp,
				  struct ast_node *node);

/* --- Value constructors -------------------------------------------------- */

static struct value val_none(void)
{
	struct value v;

	v.type  = VALUE_NONE;
	v.data.number = 0.0;
	return v;
}

static struct value val_number(double n)
{
	struct value v;

	v.type = VALUE_NUMBER;
	v.data.number = n;
	return v;
}

static struct value val_string(const char *s)
{
	struct value v;

	v.type = VALUE_STRING;
	v.data.string = strdup(s);
	return v;
}

/* --- Arithmetic ---------------------------------------------------------- */

/*
 * number_op() - Apply a binary operator to two doubles.
 *
 * Extracted so eval_binary_op stays flat — no nesting inside a switch
 * inside an if inside a function.
 */
static struct value number_op(enum token_type op,
			      double l, double r, int line)
{
	switch (op) {
	case TOKEN_PLUS: return val_number(l + r);
	case TOKEN_MINUS: return val_number(l - r);
	case TOKEN_MULTIPLY: return val_number(l * r);
	case TOKEN_DIVIDE:
		if (r == 0.0) {
			fprintf(stderr,
				"runtime error: division by zero "
				"at line %d\n", line);
			return val_none();
		}
		return val_number(l / r);
	case TOKEN_EQUAL: return val_number(l == r);
	case TOKEN_NOT_EQUAL: return val_number(l != r);
	case TOKEN_LESS: return val_number(l <  r);
	case TOKEN_GREATER: return val_number(l >  r);
	case TOKEN_LESS_EQUAL: return val_number(l <= r);
	case TOKEN_GREATER_EQUAL: return val_number(l >= r);
	default:
		fprintf(stderr,
			"runtime error: unknown operator "
			"at line %d\n", line);
		return val_none();
	}
}

static struct value string_concat(const char *a, const char *b)
{
	struct value result;
	int len;

	len = (int)(strlen(a) + strlen(b)) + 1;
	result.type        = VALUE_STRING;
	result.data.string = malloc(len);
	if (!result.data.string)
		return val_none();

	strcpy(result.data.string, a);
	strcat(result.data.string, b);
	return result;
}

static struct value eval_binary_op(struct interpreter *interp,
				   struct ast_node *node)
{
	struct value left;
	struct value right;

	left  = interpreter_evaluate(interp, node->data.binary_op.left);
	right = interpreter_evaluate(interp, node->data.binary_op.right);

	if (left.type == VALUE_NUMBER && right.type == VALUE_NUMBER)
		return number_op(node->data.binary_op.op,
				 left.data.number,
				 right.data.number,
				 node->line_number);

	if (left.type  == VALUE_STRING  &&
	    right.type == VALUE_STRING  &&
	    node->data.binary_op.op == TOKEN_PLUS)
		return string_concat(left.data.string,
				     right.data.string);

	fprintf(stderr, "runtime error: type mismatch at line %d\n",
		node->line_number);
	return val_none();
}

static struct value eval_unary_op(struct interpreter *interp,
				  struct ast_node *node)
{
	struct value operand;

	operand = interpreter_evaluate(interp,
				       node->data.unary_op.operand);

	if (operand.type != VALUE_NUMBER) {
		fprintf(stderr,
			"runtime error: unary op on non-number "
			"at line %d\n", node->line_number);
		return val_none();
	}

	switch (node->data.unary_op.op) {
	case TOKEN_MINUS:	return val_number(-operand.data.number);
	case TOKEN_PLUS:	return val_number(+operand.data.number);
	default:
		fprintf(stderr,
			"runtime error: unknown unary op "
			"at line %d\n", node->line_number);
		return val_none();
	}
}

/* --- Function calls ------------------------------------------------------ */

/*
 * bind_args() - Evaluate call arguments in caller scope then bind
 *               them into func_scope.
 *
 * Must happen before the scope switch so argument expressions resolve
 * against the caller's names, not the empty function scope.
 */
static void bind_args(struct interpreter *interp,
		      struct ast_node *call,
		      struct ast_node *def,
		      struct symbol_table *func_scope)
{
	struct value arg_values[MAX_ARGS];
	int nargs;
	int nparams;
	int j;

	nargs = call->data.function_call.arg_count;
	if (nargs > MAX_ARGS)
		nargs = MAX_ARGS;

	for (j = 0; j < nargs; j++)
		arg_values[j] = interpreter_evaluate(
			interp,
			call->data.function_call.arguments[j]);

	nparams = def->data.function_def.param_count;
	for (j = 0; j < nparams && j < nargs; j++)
		symbol_table_set_local(
			func_scope,
			def->data.function_def.parameters[j],
			arg_values[j]);
}

static struct value eval_function_call(struct interpreter *interp,
				       struct ast_node *node)
{
	struct symbol *func_sym;
	struct ast_node	*func_def;
	struct symbol_table *func_scope;
	struct symbol_table *saved_scope;
	int saved_returned;
	struct value saved_return;
	struct value result;
	const char *fname;

	fname    = node->data.function_call.function_name;
	func_sym = symbol_table_find(interp->current_scope, fname);

	if (!func_sym || func_sym->value.type != VALUE_FUNCTION) {
		fprintf(stderr,
			"runtime error: undefined function '%s' "
			"at line %d\n", fname, node->line_number);
		return val_none();
	}

	if (interp->call_depth >= MAX_CALL_DEPTH) {
		fprintf(stderr,
			"runtime error: max recursion depth (%d) "
			"exceeded at line %d\n",
			MAX_CALL_DEPTH, node->line_number);
		return val_none();
	}

	func_def   = func_sym->value.data.function;
	func_scope = symbol_table_create(interp->current_scope);
	if (!func_scope)
		return val_none();

	bind_args(interp, node, func_def, func_scope);

	saved_scope     = interp->current_scope;
	saved_returned  = interp->has_returned;
	saved_return    = interp->return_value;

	interp->current_scope = func_scope;
	interp->has_returned  = 0;
	interp->return_value  = val_none();
	interp->call_depth++;

	interpreter_evaluate(interp, func_def->data.function_def.body);
	result = interp->return_value;

	interp->call_depth--;
	interp->current_scope = saved_scope;
	interp->has_returned  = saved_returned;
	interp->return_value  = saved_return;

	symbol_table_destroy(func_scope);
	return result;
}

/* --- Print --------------------------------------------------------------- */

static void print_value(struct value v)
{
	switch (v.type) {
	case VALUE_NUMBER:
		if (v.data.number == (int)v.data.number)
			printf("%.0f\n", v.data.number);
		else
			printf("%g\n", v.data.number);
		break;
	case VALUE_STRING:
		printf("%s\n", v.data.string);
		break;
	case VALUE_NONE:
		printf("None\n");
		break;
	default:
		printf("<unknown>\n");
		break;
	}
}

/* --- Public API ---------------------------------------------------------- */

/**
 * interpreter_create() - Allocate and initialise a new interpreter.
 */
struct interpreter *interpreter_create(void)
{
	struct interpreter *interp;

	interp = calloc(1, sizeof(*interp));
	if (!interp) {
		fprintf(stderr, "interpreter: out of memory\n");
		return NULL;
	}

	interp->global_scope  = symbol_table_create(NULL);
	if (!interp->global_scope) {
		free(interp);
		return NULL;
	}

	interp->current_scope = interp->global_scope;
	interp->has_returned  = 0;
	interp->return_value  = val_none();
	interp->call_depth    = 0;
	return interp;
}

/**
 * interpreter_destroy() - Free an interpreter and its global scope.
 */
void interpreter_destroy(struct interpreter *interp)
{
	if (!interp)
		return;
	symbol_table_destroy(interp->global_scope);
	free(interp);
}

/**
 * interpreter_evaluate() - Recursively evaluate an AST node.
 */
struct value interpreter_evaluate(struct interpreter *interp,
				  struct ast_node *node)
{
	struct value result = val_none();
	struct symbol *sym;
	struct value cond;
	struct value value;
	struct value fv;
	int is_true;
	int j;

	if (!interp || !node)
		return result;

	switch (node->type) {
	case AST_NUMBER:
		return val_number(node->data.number.value);

	case AST_STRING:
		return val_string(node->data.string.value);

	case AST_IDENTIFIER:
		sym = symbol_table_find(interp->current_scope,
					node->data.identifier.name);
		if (sym)
			return sym->value;
		fprintf(stderr,
			"runtime error: undefined variable '%s' "
			"at line %d\n",
			node->data.identifier.name,
			node->line_number);
		return val_none();

	case AST_BINARY_OP:
		return eval_binary_op(interp, node);

	case AST_UNARY_OP:
		return eval_unary_op(interp, node);

	case AST_ASSIGNMENT:
		value = interpreter_evaluate(
			interp, node->data.assignment.value);
		symbol_table_set(interp->current_scope,
				 node->data.assignment.variable,
				 value);
		return value;

	case AST_IF_STMT:
		cond = interpreter_evaluate(
			interp, node->data.if_stmt.condition);
		is_true = (cond.type == VALUE_NUMBER &&
			   cond.data.number != 0.0);
		if (is_true)
			return interpreter_evaluate(
				interp,
				node->data.if_stmt.then_block);
		if (node->data.if_stmt.else_block)
			return interpreter_evaluate(
				interp,
				node->data.if_stmt.else_block);
		return val_none();

	case AST_WHILE_STMT:
		for (;;) {
			cond = interpreter_evaluate(
				interp,
				node->data.while_stmt.condition);
			is_true = (cond.type == VALUE_NUMBER &&
				   cond.data.number != 0.0);
			if (!is_true || interp->has_returned)
				break;
			interpreter_evaluate(
				interp,
				node->data.while_stmt.body);
		}
		return val_none();

	case AST_FUNCTION_DEF:
		fv.type          = VALUE_FUNCTION;
		fv.data.function = node;
		symbol_table_set(interp->current_scope,
				 node->data.function_def.name, fv);
		return val_none();

	case AST_FUNCTION_CALL:
		return eval_function_call(interp, node);

	case AST_RETURN_STMT:
		if (node->data.return_stmt.value)
			interp->return_value = interpreter_evaluate(
				interp,
				node->data.return_stmt.value);
		else
			interp->return_value = val_none();
		interp->has_returned = 1;
		return interp->return_value;

	case AST_PRINT_STMT:
		value = interpreter_evaluate(
			interp, node->data.print_stmt.value);
		print_value(value);
		return val_none();

	case AST_BLOCK:
		for (j = 0;
		     j < node->data.block.count &&
		     !interp->has_returned; j++)
			result = interpreter_evaluate(
				interp,
				node->data.block.statements[j]);
		return result;

	case AST_PROGRAM:
		for (j = 0; j < node->data.program.count; j++)
			result = interpreter_evaluate(
				interp,
				node->data.program.statements[j]);
		return result;

	default:
		fprintf(stderr,
			"runtime error: unknown node type %d "
			"at line %d\n",
			node->type, node->line_number);
		return val_none();
	}
}
