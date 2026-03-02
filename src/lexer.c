/* SPDX-License-Identifier: MIT */
#include "utils.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INDENT_INIT_CAP	32
#define STRING_BUF_CAP 1024
#define IDENT_BUF_CAP 256
#define NUM_BUF_CAP 64

/**
 * lexer_create() - Allocate and initialise a lexer.
 */
struct lexer *lexer_create(const char *source)
{
	struct lexer *lex;

	if (!source)
		return NULL;

	lex = calloc(1, sizeof(*lex));
	if (!lex)
		goto err_lex;

	lex->indent_stack = malloc(sizeof(int) * INDENT_INIT_CAP);
	if (!lex->indent_stack)
		goto err_stack;

	lex->source = source;
	lex->source_len = (int)strlen(source);
	lex->position = 0;
	lex->line = 1;
	lex->column = 1;
	lex->indent_stack[0] = 0;
	lex->indent_top = 0;
	lex->indent_capacity = INDENT_INIT_CAP;
	lex->at_line_start = 1;
	lex->pending_dedents = 0;
	return lex;

err_stack:
	free(lex);
err_lex:
	fprintf(stderr, "lexer: out of memory\n");
	return NULL;
}

/**
 * lexer_destroy() - Free a lexer and its internal buffers.
 */
void lexer_destroy(struct lexer *lex)
{
	if (!lex)
		return;
	free(lex->indent_stack);
	free(lex);
}

/* --- Internal helpers ---------------------------------------------------- */

static char lex_peek(const struct lexer *lex)
{
	if (lex->position >= lex->source_len)
		return '\0';
	return lex->source[lex->position];
}

static char lex_advance(struct lexer *lex)
{
	char c;

	if (lex->position >= lex->source_len)
		return '\0';

	c = lex->source[lex->position++];
	if (c == '\n') {
		lex->line++;
		lex->column      = 1;
		lex->at_line_start = 1;
	} else {
		lex->column++;
	}

	return c;
}

static void lex_skip_ws(struct lexer *lex)
{
	while (lex_peek(lex) == ' ' || lex_peek(lex) == '\t')
		lex_advance(lex);
}

static void lex_skip_comment(struct lexer *lex)
{
	while (lex_peek(lex) != '\n' && lex_peek(lex) != '\0')
		lex_advance(lex);
}

static struct token make_tok(enum token_type type, const char *value,
			     int line, int col)
{
	struct token t;

	t.type = type;
	t.value = strdup(value);
	t.line = line;
	t.column = col;
	t.number = 0.0;
	return t;
}

static enum token_type classify_keyword(const char *s)
{
	if (!strcmp(s, "if"))     return TOKEN_IF;
	if (!strcmp(s, "else"))   return TOKEN_ELSE;
	if (!strcmp(s, "while"))  return TOKEN_WHILE;
	if (!strcmp(s, "def"))    return TOKEN_DEF;
	if (!strcmp(s, "return")) return TOKEN_RETURN;
	if (!strcmp(s, "print"))  return TOKEN_PRINT;

	return TOKEN_IDENTIFIER;
}

static struct token read_number(struct lexer *lex)
{
	char buf[NUM_BUF_CAP];
	int j = 0;
	int has_dot = 0;
	int line = lex->line;
	int col = lex->column;
	struct token t;

	while (j < NUM_BUF_CAP - 1 &&
	       (isdigit(lex_peek(lex)) ||
		(lex_peek(lex) == '.' && !has_dot))) {
		if (lex_peek(lex) == '.')
			has_dot = 1;
		buf[j++] = lex_advance(lex);
	}
	buf[j] = '\0';

	t = make_tok(TOKEN_NUMBER, buf, line, col);
	t.number = atof(buf);

	return t;
}

static struct token read_identifier(struct lexer *lex)
{
	char buf[IDENT_BUF_CAP];
	int  j    = 0;
	int  line = lex->line;
	int  col  = lex->column;

	while (j < IDENT_BUF_CAP - 1 &&
	       (isalnum(lex_peek(lex)) || lex_peek(lex) == '_'))
		buf[j++] = lex_advance(lex);
	buf[j] = '\0';

	return make_tok(classify_keyword(buf), buf, line, col);
}

static struct token read_string(struct lexer *lex)
{
	char buf[STRING_BUF_CAP];
	int  j  = 0;
	int  line = lex->line;
	int  col = lex->column;
	char quote = lex_advance(lex);
	char esc;

	while (lex_peek(lex) != quote &&
	       lex_peek(lex) != '\0'  &&
	       j < STRING_BUF_CAP - 1) {
		if (lex_peek(lex) != '\\') {
			buf[j++] = lex_advance(lex);
			continue;
		}
		lex_advance(lex);
		esc = lex_advance(lex);
		switch (esc) {
		case 'n':  buf[j++] = '\n'; break;
		case 't':  buf[j++] = '\t'; break;
		case 'r':  buf[j++] = '\r'; break;
		case '\\': buf[j++] = '\\'; break;
		case '"':  buf[j++] = '"';  break;
		case '\'': buf[j++] = '\''; break;
		default:   buf[j++] = esc;  break;
		}
	}

	if (lex_peek(lex) == quote)
		lex_advance(lex);

	buf[j] = '\0';
	return make_tok(TOKEN_STRING, buf, line, col);
}

/* --- Indentation handling ------------------------------------------------ */

static int is_blank_line(const struct lexer *lex, int tmp_pos)
{
	char c;

	if (tmp_pos >= lex->source_len)
		return 1;
	c = lex->source[tmp_pos];
	return (c == '\n' || c == '#' || c == '\0');
}

static int count_indent(const struct lexer *lex, int *out_pos)
{
	int spaces = 0;
	int tmp_pos = lex->position;

	while (tmp_pos < lex->source_len &&
	       (lex->source[tmp_pos] == ' ' ||
		lex->source[tmp_pos] == '\t')) {
		spaces += (lex->source[tmp_pos] == '\t') ? 4 : 1;
		tmp_pos++;
	}

	*out_pos = tmp_pos;
	return spaces;
}

static int push_indent(struct lexer *lex, int spaces)
{
	int *new_stack;
	int new_cap;

	if (lex->indent_top + 1 < lex->indent_capacity) {
		lex->indent_stack[++lex->indent_top] = spaces;
		return 1;
	}

	new_cap   = lex->indent_capacity * 2;
	new_stack = realloc(lex->indent_stack, sizeof(int) * new_cap);
	if (!new_stack) {
		fprintf(stderr, "lexer: indent stack overflow\n");
		return 0;
	}

	lex->indent_stack                    = new_stack;
	lex->indent_capacity                 = new_cap;
	lex->indent_stack[++lex->indent_top] = spaces;
	return 1;
}

/*
 * handle_line_start() - Emit INDENT or DEDENT at the start of a line.
 *
 * Sets *emitted to 1 and returns the token if the indentation level
 * changed.  Sets *emitted to 0 and returns a dummy token otherwise.
 * Multiple DEDENT levels are queued in lex->pending_dedents.
 */
static struct token handle_line_start(struct lexer *lex, int *emitted)
{
	struct token dummy = make_tok(TOKEN_EOF, "EOF", lex->line, 1);
	int tmp_pos;
	int spaces;
	int current;
	int dedents;

	spaces  = count_indent(lex, &tmp_pos);
	*emitted = 0;

	if (is_blank_line(lex, tmp_pos)) {
		lex_skip_ws(lex);
		lex->at_line_start = 0;
		return dummy;
	}

	current = lex->indent_stack[lex->indent_top];

	if (spaces == current) {
		lex_skip_ws(lex);
		lex->at_line_start = 0;
		return dummy;
	}

	if (spaces > current) {
		lex_skip_ws(lex);
		lex->at_line_start = 0;
		if (!push_indent(lex, spaces))
			return dummy;
		*emitted = 1;
		return make_tok(TOKEN_INDENT, "INDENT", lex->line, 1);
	}

	/* Dedent: pop levels and queue extras. */
	lex_skip_ws(lex);
	lex->at_line_start = 0;
	dedents = 0;
	while (lex->indent_top > 0 &&
	       lex->indent_stack[lex->indent_top] > spaces) {
		lex->indent_top--;
		dedents++;
	}

	lex->pending_dedents = dedents - 1;
	*emitted = 1;
	return make_tok(TOKEN_DEDENT, "DEDENT", lex->line, 1);
}

/* --- Two-character operator dispatch ------------------------------------ */

static struct token try_two_char(struct lexer *lex, char first,
				 int line, int col)
{
	char next = lex_peek(lex);

	if (next != '=')
		return make_tok(TOKEN_ERROR, "?", line, col);

	switch (first) {
	case '=':
		lex_advance(lex);
		return make_tok(TOKEN_EQUAL, "==", line, col);
	case '!':
		lex_advance(lex);
		return make_tok(TOKEN_NOT_EQUAL, "!=", line, col);
	case '<':
		lex_advance(lex);
		return make_tok(TOKEN_LESS_EQUAL, "<=", line, col);
	case '>':
		lex_advance(lex);
		return make_tok(TOKEN_GREATER_EQUAL, ">=", line, col);
	default:
		return make_tok(TOKEN_ERROR, "?", line, col);
	}
}

/* --- Public API ---------------------------------------------------------- */

/**
 * lexer_next_token() - Return the next token from the source stream.
 */
struct token lexer_next_token(struct lexer *lex)
{
	struct token tok;
	int emitted;
	int line;
	int col;
	char c;
	char buf[2];

	if (!lex)
		return make_tok(TOKEN_ERROR, "?", 0, 0);

	/* Drain queued DEDENT tokens first. */
	if (lex->pending_dedents > 0) {
		lex->pending_dedents--;
		return make_tok(TOKEN_DEDENT, "DEDENT", lex->line, 1);
	}

	if (lex->at_line_start) {
		tok = handle_line_start(lex, &emitted);
		if (emitted)
			return tok;
	}

	lex_skip_ws(lex);

	while (lex_peek(lex) == '#') {
		lex_skip_comment(lex);
		lex_skip_ws(lex);
	}

	line = lex->line;
	col  = lex->column;
	c    = lex_peek(lex);

	if (c == '\0') {
		if (lex->indent_top > 0) {
			lex->indent_top--;
			return make_tok(TOKEN_DEDENT, "DEDENT", line, col);
		}
		return make_tok(TOKEN_EOF, "EOF", line, col);
	}

	if (c == '\n') {
		lex_advance(lex);
		return make_tok(TOKEN_NEWLINE, "\n", line, col);
	}

	if (isdigit(c))
		return read_number(lex);

	if (isalpha(c) || c == '_')
		return read_identifier(lex);

	if (c == '"' || c == '\'')
		return read_string(lex);

	lex_advance(lex);

	/* Try two-character operators first. */
	if (c == '=' || c == '!' || c == '<' || c == '>') {
		tok = try_two_char(lex, c, line, col);
		if (tok.type != TOKEN_ERROR)
			return tok;
		free(tok.value);
	}

	/* Single-character operators. */
	switch (c) {
	case '+': return make_tok(TOKEN_PLUS,     "+", line, col);
	case '-': return make_tok(TOKEN_MINUS,    "-", line, col);
	case '*': return make_tok(TOKEN_MULTIPLY, "*", line, col);
	case '/': return make_tok(TOKEN_DIVIDE,   "/", line, col);
	case '(': return make_tok(TOKEN_LPAREN,   "(", line, col);
	case ')': return make_tok(TOKEN_RPAREN,   ")", line, col);
	case ',': return make_tok(TOKEN_COMMA,    ",", line, col);
	case ':': return make_tok(TOKEN_COLON,    ":", line, col);
	case '=': return make_tok(TOKEN_ASSIGN,   "=", line, col);
	case '<': return make_tok(TOKEN_LESS,     "<", line, col);
	case '>': return make_tok(TOKEN_GREATER,  ">", line, col);
	default:
		break;
	}

	buf[0] = c;
	buf[1] = '\0';
	return make_tok(TOKEN_ERROR, buf, line, col);
}
