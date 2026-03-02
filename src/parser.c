#include "utils.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PARAMS	64
#define MAX_ARGS	64
#define BLOCK_INIT_CAP	16
#define PROG_INIT_CAP	64

/* Forward declarations — grammar is mutually recursive. */
static struct ast_node *parse_expression(struct parser *p);
static struct ast_node *parse_statement(struct parser *p);
static struct ast_node *parse_block(struct parser *p);

/* --- Parser utilities ---------------------------------------------------- */

/**
 * parser_create() - Initialise a parser over a token array.
 */
struct parser *parser_create(struct token *tokens, int count)
{
	struct parser *p;

	if (!tokens || count <= 0)
		return NULL;

	p = calloc(1, sizeof(*p));
	if (!p) {
		fprintf(stderr, "parser: out of memory\n");
		return NULL;
	}

	p->tokens      = tokens;
	p->position    = 0;
	p->token_count = count;
	return p;
}

/**
 * parser_destroy() - Free the parser struct (not the token array).
 */
void parser_destroy(struct parser *p)
{
	free(p);
}

static struct token *cur(const struct parser *p)
{
	static struct token eof_tok = {
		TOKEN_EOF, "EOF", 0, 0, 0.0
	};

	if (!p || p->position >= p->token_count)
		return &eof_tok;
	return &p->tokens[p->position];
}

static void advance(struct parser *p)
{
	if (p && p->position < p->token_count)
		p->position++;
}

static int match(const struct parser *p, enum token_type type)
{
	return cur(p)->type == type;
}

static int consume(struct parser *p, enum token_type type)
{
	if (!match(p, type))
		return 0;
	advance(p);
	return 1;
}

static void skip_newlines(struct parser *p)
{
	while (match(p, TOKEN_NEWLINE))
		advance(p);
}

/* --- Statement list helpers --------------------------------------------- */

/*
 * append_stmt() - Append a statement to a growing list.
 *
 * Doubles capacity via realloc when the list is full.
 * Returns 0 on allocation failure.
 */
static int append_stmt(struct ast_node ***stmts, int *count,
		       int *capacity, struct ast_node *stmt)
{
	struct ast_node **new_list;
	int new_cap;

	if (*count < *capacity) {
		(*stmts)[(*count)++] = stmt;
		return 1;
	}

	new_cap  = (*capacity) * 2;
	new_list = realloc(*stmts, sizeof(struct ast_node *) * new_cap);
	if (!new_list) {
		fprintf(stderr, "parser: out of memory\n");
		return 0;
	}

	*stmts    = new_list;
	*capacity = new_cap;
	(*stmts)[(*count)++] = stmt;
	return 1;
}

/* --- Expression parsing -------------------------------------------------- */

static struct ast_node *parse_call_args(struct parser *p,
					struct ast_node *call,
					const struct token *tok)
{
	struct ast_node *arg;

	advance(p); /* consume '(' */

	if (match(p, TOKEN_RPAREN)) {
		consume(p, TOKEN_RPAREN);
		return call;
	}

	do {
		if (call->data.function_call.arg_count >= MAX_ARGS) {
			fprintf(stderr,
				"parse error: too many args "
				"at line %d\n", tok->line);
			ast_free(call);
			return NULL;
		}
		arg = parse_expression(p);
		if (!arg) {
			ast_free(call);
			return NULL;
		}
		call->data.function_call.arguments[
			call->data.function_call.arg_count++] = arg;
	} while (consume(p, TOKEN_COMMA));

	consume(p, TOKEN_RPAREN);
	return call;
}

static struct ast_node *parse_identifier_or_call(struct parser *p)
{
	struct token *tok = cur(p);
	struct ast_node *call;

	advance(p);

	if (!match(p, TOKEN_LPAREN))
		return ast_create_identifier(tok->value, tok->line);

	call = ast_create_node(AST_FUNCTION_CALL, tok->line);
	if (!call)
		return NULL;

	call->data.function_call.function_name = strdup(tok->value);
	call->data.function_call.arguments =
		malloc(sizeof(struct ast_node *) * MAX_ARGS);
	call->data.function_call.arg_count = 0;

	if (!call->data.function_call.function_name ||
	    !call->data.function_call.arguments) {
		ast_free(call);
		return NULL;
	}

	return parse_call_args(p, call, tok);
}

static struct ast_node *parse_primary(struct parser *p)
{
	struct token *tok = cur(p);
	struct ast_node *expr;

	switch (tok->type) {
	case TOKEN_NUMBER:
		advance(p);
		return ast_create_number(tok->number, tok->line);
	case TOKEN_STRING:
		advance(p);
		return ast_create_string(tok->value, tok->line);
	case TOKEN_IDENTIFIER:
		return parse_identifier_or_call(p);
	case TOKEN_LPAREN:
		advance(p);
		expr = parse_expression(p);
		consume(p, TOKEN_RPAREN);
		return expr;
	default:
		fprintf(stderr,
			"parse error: unexpected token '%s' "
			"at line %d\n",
			tok->value, tok->line);
		return NULL;
	}
}

static struct ast_node *parse_unary(struct parser *p)
{
	struct token *op;
	struct ast_node *node;
	struct ast_node *operand;

	if (!match(p, TOKEN_MINUS) && !match(p, TOKEN_PLUS))
		return parse_primary(p);

	op = cur(p);
	advance(p);

	operand = parse_unary(p);
	if (!operand)
		return NULL;

	node = ast_create_node(AST_UNARY_OP, op->line);
	if (!node) {
		ast_free(operand);
		return NULL;
	}

	node->data.unary_op.op      = op->type;
	node->data.unary_op.operand = operand;
	return node;
}

static struct ast_node *parse_term(struct parser *p)
{
	struct ast_node *left;
	struct ast_node *right;
	struct ast_node *node;
	struct token *op;

	left = parse_unary(p);
	if (!left)
		return NULL;

	while (match(p, TOKEN_MULTIPLY) || match(p, TOKEN_DIVIDE)) {
		op    = cur(p);
		advance(p);
		right = parse_unary(p);
		if (!right) {
			ast_free(left);
			return NULL;
		}
		node = ast_create_binary_op(left, op->type, right,
					    op->line);
		if (!node) {
			ast_free(left);
			ast_free(right);
			return NULL;
		}
		left = node;
	}

	return left;
}

static struct ast_node *parse_arithmetic(struct parser *p)
{
	struct ast_node *left;
	struct ast_node *right;
	struct ast_node *node;
	struct token *op;

	left = parse_term(p);
	if (!left)
		return NULL;

	while (match(p, TOKEN_PLUS) || match(p, TOKEN_MINUS)) {
		op    = cur(p);
		advance(p);
		right = parse_term(p);
		if (!right) {
			ast_free(left);
			return NULL;
		}
		node = ast_create_binary_op(left, op->type, right,
					    op->line);
		if (!node) {
			ast_free(left);
			ast_free(right);
			return NULL;
		}
		left = node;
	}

	return left;
}

static int is_comparison_op(enum token_type t)
{
	return (t == TOKEN_EQUAL        ||
		t == TOKEN_NOT_EQUAL    ||
		t == TOKEN_LESS         ||
		t == TOKEN_GREATER      ||
		t == TOKEN_LESS_EQUAL   ||
		t == TOKEN_GREATER_EQUAL);
}

static struct ast_node *parse_comparison(struct parser *p)
{
	struct ast_node *left;
	struct ast_node *right;
	struct ast_node *node;
	struct token    *op;

	left = parse_arithmetic(p);
	if (!left)
		return NULL;

	while (is_comparison_op(cur(p)->type)) {
		op    = cur(p);
		advance(p);
		right = parse_arithmetic(p);
		if (!right) {
			ast_free(left);
			return NULL;
		}
		node = ast_create_binary_op(left, op->type, right,
					    op->line);
		if (!node) {
			ast_free(left);
			ast_free(right);
			return NULL;
		}
		left = node;
	}

	return left;
}

static struct ast_node *parse_expression(struct parser *p)
{
	return parse_comparison(p);
}

/* --- Statement parsing --------------------------------------------------- */

static struct ast_node *parse_if_stmt(struct parser *p)
{
	struct token *tok = cur(p);
	struct ast_node *condition;
	struct ast_node *then_block;
	struct ast_node *else_block = NULL;
	struct ast_node *node;

	advance(p); /* consume 'if' */

	condition = parse_expression(p);
	if (!condition)
		return NULL;

	if (!consume(p, TOKEN_COLON)) {
		fprintf(stderr,
			"parse error: expected ':' after if "
			"at line %d\n", tok->line);
		ast_free(condition);
		return NULL;
	}

	skip_newlines(p);
	then_block = parse_block(p);
	if (!then_block) {
		ast_free(condition);
		return NULL;
	}

	skip_newlines(p);
	if (match(p, TOKEN_ELSE)) {
		advance(p);
		if (!consume(p, TOKEN_COLON)) {
			fprintf(stderr,
				"parse error: expected ':' after else "
				"at line %d\n", cur(p)->line);
			ast_free(condition);
			ast_free(then_block);
			return NULL;
		}
		skip_newlines(p);
		else_block = parse_block(p);
		if (!else_block) {
			ast_free(condition);
			ast_free(then_block);
			return NULL;
		}
	}

	node = ast_create_node(AST_IF_STMT, tok->line);
	if (!node)
		goto err;

	node->data.if_stmt.condition  = condition;
	node->data.if_stmt.then_block = then_block;
	node->data.if_stmt.else_block = else_block;
	return node;

err:
	ast_free(condition);
	ast_free(then_block);
	ast_free(else_block);
	return NULL;
}

static struct ast_node *parse_while_stmt(struct parser *p)
{
	struct token *tok = cur(p);
	struct ast_node *condition;
	struct ast_node *body;
	struct ast_node *node;

	advance(p); /* consume 'while' */

	condition = parse_expression(p);
	if (!condition)
		return NULL;

	if (!consume(p, TOKEN_COLON)) {
		fprintf(stderr,
			"parse error: expected ':' after while "
			"at line %d\n", tok->line);
		ast_free(condition);
		return NULL;
	}

	skip_newlines(p);
	body = parse_block(p);
	if (!body) {
		ast_free(condition);
		return NULL;
	}

	node = ast_create_node(AST_WHILE_STMT, tok->line);
	if (!node)
		goto err;

	node->data.while_stmt.condition = condition;
	node->data.while_stmt.body      = body;
	return node;

err:
	ast_free(condition);
	ast_free(body);
	return NULL;
}

static int parse_param_list(struct parser *p, struct ast_node *node)
{
	struct token *param_tok;
	int j;

	if (match(p, TOKEN_RPAREN))
		return 1;

	do {
		if (!match(p, TOKEN_IDENTIFIER)) {
			fprintf(stderr,
				"parse error: expected param name "
				"at line %d\n", cur(p)->line);
			return 0;
		}
		if (node->data.function_def.param_count >= MAX_PARAMS) {
			fprintf(stderr,
				"parse error: too many params "
				"at line %d\n", cur(p)->line);
			return 0;
		}
		param_tok = cur(p);
		advance(p);
		j = node->data.function_def.param_count;
		node->data.function_def.parameters[j] =
			strdup(param_tok->value);
		if (!node->data.function_def.parameters[j])
			return 0;
		node->data.function_def.param_count++;
	} while (consume(p, TOKEN_COMMA));

	return 1;
}

static struct ast_node *parse_function_def(struct parser *p)
{
	struct token	*def_tok = cur(p);
	struct token	*name_tok;
	struct ast_node *node;
	struct ast_node *body;

	advance(p); /* consume 'def' */

	if (!match(p, TOKEN_IDENTIFIER)) {
		fprintf(stderr,
			"parse error: expected function name "
			"at line %d\n", def_tok->line);
		return NULL;
	}

	name_tok = cur(p);
	advance(p);

	if (!consume(p, TOKEN_LPAREN)) {
		fprintf(stderr,
			"parse error: expected '(' "
			"at line %d\n", name_tok->line);
		return NULL;
	}

	node = ast_create_node(AST_FUNCTION_DEF, def_tok->line);
	if (!node)
		return NULL;

	node->data.function_def.name =
		strdup(name_tok->value);
	node->data.function_def.parameters =
		malloc(sizeof(char *) * MAX_PARAMS);
	node->data.function_def.param_count = 0;

	if (!node->data.function_def.name ||
	    !node->data.function_def.parameters)
		goto err;

	if (!parse_param_list(p, node))
		goto err;

	if (!consume(p, TOKEN_RPAREN) || !consume(p, TOKEN_COLON)) {
		fprintf(stderr,
			"parse error: expected ')' and ':' "
			"at line %d\n", name_tok->line);
		goto err;
	}

	skip_newlines(p);
	body = parse_block(p);
	if (!body)
		goto err;

	node->data.function_def.body = body;
	return node;

err:
	ast_free(node);
	return NULL;
}

static struct ast_node *parse_return_stmt(struct parser *p)
{
	struct token	*tok = cur(p);
	struct ast_node *node;

	advance(p); /* consume 'return' */

	node = ast_create_node(AST_RETURN_STMT, tok->line);
	if (!node)
		return NULL;

	if (match(p, TOKEN_NEWLINE) ||
	    match(p, TOKEN_DEDENT)  ||
	    match(p, TOKEN_EOF)) {
		node->data.return_stmt.value = NULL;
		return node;
	}

	node->data.return_stmt.value = parse_expression(p);
	if (!node->data.return_stmt.value) {
		ast_free(node);
		return NULL;
	}

	return node;
}

static struct ast_node *parse_print_stmt(struct parser *p)
{
	struct token *tok = cur(p);
	struct ast_node *node;
	struct ast_node *value;

	advance(p); /* consume 'print' */

	if (!consume(p, TOKEN_LPAREN)) {
		fprintf(stderr,
			"parse error: expected '(' after print "
			"at line %d\n", tok->line);
		return NULL;
	}

	value = parse_expression(p);
	if (!value)
		return NULL;

	if (!consume(p, TOKEN_RPAREN)) {
		fprintf(stderr,
			"parse error: expected ')' closing print "
			"at line %d\n", tok->line);
		ast_free(value);
		return NULL;
	}

	node = ast_create_node(AST_PRINT_STMT, tok->line);
	if (!node) {
		ast_free(value);
		return NULL;
	}

	node->data.print_stmt.value = value;
	return node;
}

static struct ast_node *parse_assignment(struct parser *p)
{
	struct token *id_tok = cur(p);
	struct ast_node *node;
	struct ast_node *value;

	advance(p); /* consume identifier */
	advance(p); /* consume '='        */

	value = parse_expression(p);
	if (!value)
		return NULL;

	node = ast_create_node(AST_ASSIGNMENT, id_tok->line);
	if (!node) {
		ast_free(value);
		return NULL;
	}

	node->data.assignment.variable = strdup(id_tok->value);
	if (!node->data.assignment.variable) {
		ast_free(node);
		ast_free(value);
		return NULL;
	}

	node->data.assignment.value = value;
	return node;
}

static int next_is_assign(const struct parser *p)
{
	return (p->position + 1 < p->token_count &&
		p->tokens[p->position + 1].type == TOKEN_ASSIGN);
}

static struct ast_node *parse_statement(struct parser *p)
{
	skip_newlines(p);

	if (match(p, TOKEN_EOF) || match(p, TOKEN_DEDENT))
		return NULL;

	switch (cur(p)->type) {
	case TOKEN_IF:		return parse_if_stmt(p);
	case TOKEN_WHILE:	return parse_while_stmt(p);
	case TOKEN_DEF:		return parse_function_def(p);
	case TOKEN_RETURN:	return parse_return_stmt(p);
	case TOKEN_PRINT:	return parse_print_stmt(p);
	case TOKEN_IDENTIFIER:
		if (next_is_assign(p))
			return parse_assignment(p);
		return parse_expression(p);
	default:
		return parse_expression(p);
	}
}

/* --- Block and program --------------------------------------------------- */

static struct ast_node *parse_block(struct parser *p)
{
	struct ast_node	 *block;
	struct ast_node	 *stmt;

	block = ast_create_node(AST_BLOCK, cur(p)->line);
	if (!block)
		return NULL;

	block->data.block.statements =
		malloc(sizeof(struct ast_node *) * BLOCK_INIT_CAP);
	block->data.block.count    = 0;
	block->data.block.capacity = BLOCK_INIT_CAP;

	if (!block->data.block.statements) {
		ast_free(block);
		return NULL;
	}

	if (!consume(p, TOKEN_INDENT))
		return block;

	while (!match(p, TOKEN_DEDENT) && !match(p, TOKEN_EOF)) {
		stmt = parse_statement(p);
		skip_newlines(p);
		if (!stmt)
			continue;
		if (!append_stmt(&block->data.block.statements,
				 &block->data.block.count,
				 &block->data.block.capacity,
				 stmt)) {
			ast_free(block);
			return NULL;
		}
	}

	consume(p, TOKEN_DEDENT);
	return block;
}

/**
 * parser_parse_program() - Parse all top-level statements into an AST.
 */
struct ast_node *parser_parse_program(struct parser *p)
{
	struct ast_node	 *prog;
	struct ast_node	 *stmt;

	if (!p)
		return NULL;

	prog = ast_create_node(AST_PROGRAM, 1);
	if (!prog)
		return NULL;

	prog->data.program.statements =
		malloc(sizeof(struct ast_node *) * PROG_INIT_CAP);
	prog->data.program.count    = 0;
	prog->data.program.capacity = PROG_INIT_CAP;

	if (!prog->data.program.statements) {
		ast_free(prog);
		return NULL;
	}

	while (!match(p, TOKEN_EOF)) {
		stmt = parse_statement(p);
		skip_newlines(p);
		if (!stmt)
			continue;
		if (!append_stmt(&prog->data.program.statements,
				 &prog->data.program.count,
				 &prog->data.program.capacity,
				 stmt)) {
			ast_free(prog);
			return NULL;
		}
	}

	return prog;
}
