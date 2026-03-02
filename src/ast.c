#include "utils.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * ast_create_node() - Allocate and zero-initialise a new AST node.
 */
struct ast_node *ast_create_node(enum ast_node_type type, int line_number)
{
	struct ast_node *node;

	node = calloc(1, sizeof(*node));

	if (!node) {
		fprintf(stderr, "ast: out of memory\n");
		return NULL;
	}

	node->type        = type;
	node->line_number = line_number;

	return node;
}

/**
 * ast_create_number() - Convenience constructor for a numeric literal.
 */
struct ast_node *ast_create_number(double value, int line)
{
	struct ast_node *node;

	node = ast_create_node(AST_NUMBER, line);
	if (!node)
		return NULL;

	node->data.number.value = value;
	return node;
}

/**
 * ast_create_string() - Convenience constructor for a string literal.
 */
struct ast_node *ast_create_string(const char *value, int line)
{
	struct ast_node *node;

	if (!value)
		return NULL;

	node = ast_create_node(AST_STRING, line);
	if (!node)
		return NULL;

	node->data.string.value = strdup(value);
	if (!node->data.string.value) {
		free(node);
		return NULL;
	}

	return node;
}

/**
 * ast_create_identifier() - Convenience constructor for an identifier.
 */
struct ast_node *ast_create_identifier(const char *name, int line)
{
	struct ast_node *node;

	if (!name)
		return NULL;

	node = ast_create_node(AST_IDENTIFIER, line);
	if (!node)
		return NULL;

	node->data.identifier.name = strdup(name);
	if (!node->data.identifier.name) {
		free(node);
		return NULL;
	}

	return node;
}

/**
 * ast_create_binary_op() - Convenience constructor for a binary expression.
 */
struct ast_node *ast_create_binary_op(struct ast_node *left,
				      enum token_type op,
				      struct ast_node *right,
				      int line)
{
	struct ast_node *node;

	if (!left || !right)
		return NULL;

	node = ast_create_node(AST_BINARY_OP, line);
	if (!node)
		return NULL;

	node->data.binary_op.left  = left;
	node->data.binary_op.op    = op;
	node->data.binary_op.right = right;
	return node;
}

/* --- Free helpers per node type ----------------------------------------- */

static void free_function_def(struct ast_node *node)
{
	int j;

	free(node->data.function_def.name);
	for (j = 0; j < node->data.function_def.param_count; j++)
		free(node->data.function_def.parameters[j]);
	free(node->data.function_def.parameters);
	ast_free(node->data.function_def.body);
}

static void free_function_call(struct ast_node *node)
{
	int j;

	free(node->data.function_call.function_name);
	for (j = 0; j < node->data.function_call.arg_count; j++)
		ast_free(node->data.function_call.arguments[j]);
	free(node->data.function_call.arguments);
}

static void free_block(struct ast_node *node)
{
	int j;

	for (j = 0; j < node->data.block.count; j++)
		ast_free(node->data.block.statements[j]);
	free(node->data.block.statements);
}

static void free_program(struct ast_node *node)
{
	int j;

	for (j = 0; j < node->data.program.count; j++)
		ast_free(node->data.program.statements[j]);
	free(node->data.program.statements);
}

/**
 * ast_free() - Recursively release a node and all of its descendants.
 */
void ast_free(struct ast_node *node)
{
	if (!node)
		return;

	switch (node->type) {
	case AST_STRING:
		free(node->data.string.value);
		break;
	case AST_IDENTIFIER:
		free(node->data.identifier.name);
		break;
	case AST_BINARY_OP:
		ast_free(node->data.binary_op.left);
		ast_free(node->data.binary_op.right);
		break;
	case AST_UNARY_OP:
		ast_free(node->data.unary_op.operand);
		break;
	case AST_ASSIGNMENT:
		free(node->data.assignment.variable);
		ast_free(node->data.assignment.value);
		break;
	case AST_IF_STMT:
		ast_free(node->data.if_stmt.condition);
		ast_free(node->data.if_stmt.then_block);
		ast_free(node->data.if_stmt.else_block);
		break;
	case AST_WHILE_STMT:
		ast_free(node->data.while_stmt.condition);
		ast_free(node->data.while_stmt.body);
		break;
	case AST_FUNCTION_DEF:
		free_function_def(node);
		break;
	case AST_FUNCTION_CALL:
		free_function_call(node);
		break;
	case AST_RETURN_STMT:
		ast_free(node->data.return_stmt.value);
		break;
	case AST_PRINT_STMT:
		ast_free(node->data.print_stmt.value);
		break;
	case AST_BLOCK:
		free_block(node);
		break;
	case AST_PROGRAM:
		free_program(node);
		break;
	case AST_NUMBER:
		break;
	}

	free(node);
}
