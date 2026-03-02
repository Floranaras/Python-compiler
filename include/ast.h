#ifndef AST_H
#define AST_H

#include "token.h"

/**
 * enum ast_node_type - Discriminator tag for every AST node variant.
 */
enum ast_node_type {
	AST_PROGRAM,		/* root of the tree            */
	AST_NUMBER,		/* numeric literal             */
	AST_STRING,		/* string literal              */
	AST_IDENTIFIER,		/* variable reference          */
	AST_BINARY_OP,		/* left OP right               */
	AST_UNARY_OP,		/* OP operand                  */
	AST_ASSIGNMENT,		/* name = expr                 */
	AST_IF_STMT,		/* if / else                   */
	AST_WHILE_STMT,		/* while loop                  */
	AST_FUNCTION_DEF,	/* def name(params): body      */
	AST_FUNCTION_CALL,	/* name(args)                  */
	AST_RETURN_STMT,	/* return [expr]               */
	AST_PRINT_STMT,		/* print(expr)                 */
	AST_BLOCK		/* indented statement sequence */
};

/**
 * struct ast_node - A single node in the abstract syntax tree.
 * @type:        Which variant this node represents.
 * @line_number: Source line for error reporting.
 * @data:        Variant-specific payload (anonymous union).
 *
 * Every heap-allocated string inside @data is owned by the node
 * and must be released by ast_free().
 */
struct ast_node {
	enum ast_node_type	 type;
	int			 line_number;

	union {
		/* Literals */
		struct {
			double value;
		} number;

		struct {
			char *value;
		} string;

		struct {
			char *name;
		} identifier;

		/* Expressions */
		struct {
			struct ast_node		*left;
			struct ast_node		*right;
			enum token_type		 op;
		} binary_op;

		struct {
			struct ast_node		*operand;
			enum token_type		 op;
		} unary_op;

		/* Statements */
		struct {
			char			*variable;
			struct ast_node		*value;
		} assignment;

		struct {
			struct ast_node		*condition;
			struct ast_node		*then_block;
			struct ast_node		*else_block; /* NULL if absent */
		} if_stmt;

		struct {
			struct ast_node		*condition;
			struct ast_node		*body;
		} while_stmt;

		struct {
			char			 *name;
			char			**parameters;
			int			  param_count;
			struct ast_node		 *body;
		} function_def;

		struct {
			char			 *function_name;
			struct ast_node		**arguments;
			int			  arg_count;
		} function_call;

		struct {
			struct ast_node		*value; /* NULL for bare return */
		} return_stmt;

		struct {
			struct ast_node		*value;
		} print_stmt;

		/* Collections */
		struct {
			struct ast_node		**statements;
			int			  count;
			int			  capacity;
		} block;

		struct {
			struct ast_node		**statements;
			int			  count;
			int			  capacity;
		} program;
	} data;
};

/* Hard limit on function parameters and call arguments. */
#define AST_MAX_PARAMS		64

/* Initial statement capacity for blocks; doubles on overflow. */
#define AST_BLOCK_INIT_CAP	16

/**
 * ast_create_node() - Allocate and zero-initialise a new AST node.
 * @type:        Node discriminator.
 * @line_number: Source line (for diagnostics).
 *
 * Return: Pointer to node on success, NULL on allocation failure.
 */
struct ast_node *ast_create_node(enum ast_node_type type, int line_number);

/**
 * ast_create_number() - Convenience constructor for a numeric literal.
 * @value: The numeric value.
 * @line:  Source line.
 *
 * Return: Pointer to node, or NULL on failure.
 */
struct ast_node *ast_create_number(double value, int line);

/**
 * ast_create_string() - Convenience constructor for a string literal.
 * @value: String content (copied into the node).
 * @line:  Source line.
 *
 * Return: Pointer to node, or NULL on failure.
 */
struct ast_node *ast_create_string(const char *value, int line);

/**
 * ast_create_identifier() - Convenience constructor for an identifier.
 * @name: Identifier text (copied into the node).
 * @line: Source line.
 *
 * Return: Pointer to node, or NULL on failure.
 */
struct ast_node *ast_create_identifier(const char *name, int line);

/**
 * ast_create_binary_op() - Convenience constructor for a binary expression.
 * @left:  Left-hand sub-tree (ownership transferred to the new node).
 * @op:    Operator token type.
 * @right: Right-hand sub-tree (ownership transferred to the new node).
 * @line:  Source line.
 *
 * Return: Pointer to node, or NULL on failure.
 */
struct ast_node *ast_create_binary_op(struct ast_node *left,
				      enum token_type op,
				      struct ast_node *right,
				      int line);

/**
 * ast_free() - Recursively free a node and all of its descendants.
 * @node: Root of the sub-tree to free.  Safe to call with NULL.
 */
void ast_free(struct ast_node *node);

#endif /* AST_H */
