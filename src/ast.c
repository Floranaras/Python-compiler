#include "utils.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * ast_create_node() - Allocate and zero-initialise a new AST node.
 */
struct ast_node *ast_create_node(enum ast_node_type type, int line_number)
{
	struct ast_node *node = calloc(1, sizeof(*node));

	if (!node) {
		fprintf(stderr, "ast: out of memory\n");
		return NULL;
	}

	node->type		= type;
	node->line_number	= line_number;
	return node;
}

/**
 * ast_create_number() - Convenience constructor for a numeric literal.
 */
struct ast_node *ast_create_number(double value, int line)
{
	struct ast_node *node = ast_create_node(AST_NUMBER, line);

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
	struct ast_node *node = ast_create_node(AST_STRING, line);

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
	struct ast_node *node = ast_create_node(AST_IDENTIFIER, line);

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
	struct ast_node *node = ast_create_node(AST_BINARY_OP, line);

	if (!node)
		return NULL;

	node->data.binary_op.left	= left;
	node->data.binary_op.op		= op;
	node->data.binary_op.right	= right;
	return node;
}

/**
 * ast_free() - Recursively release a node and all of its descendants.
 */
void ast_free(struct ast_node *node)
{
	int i;

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
		free(node->data.function_def.name);
		for (i = 0; i < node->data.function_def.param_count; i++)
			free(node->data.function_def.parameters[i]);
		free(node->data.function_def.parameters);
		ast_free(node->data.function_def.body);
		break;

	case AST_FUNCTION_CALL:
		free(node->data.function_call.function_name);
		for (i = 0; i < node->data.function_call.arg_count; i++)
			ast_free(node->data.function_call.arguments[i]);
		free(node->data.function_call.arguments);
		break;

	case AST_RETURN_STMT:
		ast_free(node->data.return_stmt.value);
		break;

	case AST_PRINT_STMT:
		ast_free(node->data.print_stmt.value);
		break;

	case AST_BLOCK:
		for (i = 0; i < node->data.block.count; i++)
			ast_free(node->data.block.statements[i]);
		free(node->data.block.statements);
		break;

	case AST_PROGRAM:
		for (i = 0; i < node->data.program.count; i++)
			ast_free(node->data.program.statements[i]);
		free(node->data.program.statements);
		break;

	case AST_NUMBER:
		/* scalar — no heap members */
		break;
	}

	free(node);
}
