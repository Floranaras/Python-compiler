#ifndef LEXER_H
#define LEXER_H

#include "token.h"

/**
 * struct lexer - Tokeniser state for a single source string.
 * @source:          NUL-terminated source code (not owned by lexer).
 * @source_len:      Cached strlen(@source); avoids repeated O(n) calls.
 * @position:        Current byte offset into @source.
 * @line:            Current line number (1-based).
 * @column:          Current column number (1-based).
 * @indent_stack:    Stack of active indentation levels in spaces.
 * @indent_top:      Index of the top element in @indent_stack.
 * @indent_capacity: Allocated length of @indent_stack.
 * @at_line_start:   Non-zero when the next char begins a new line.
 * @pending_dedents: DEDENT tokens queued but not yet returned.
 */
struct lexer {
	const char *source;
	int source_len;
	int position;
	int line;
	int column;
	int *indent_stack;
	int indent_top;
	int indent_capacity;
	int at_line_start;
	int pending_dedents;
};

/**
 * lexer_create() - Allocate and initialise a lexer for @source.
 * @source: NUL-terminated source string.  The lexer does not take
 *          ownership; caller must ensure it outlives the lexer.
 *
 * Return: Pointer to lexer, or NULL on allocation failure.
 */
struct lexer *lexer_create(const char *source);

/**
 * lexer_destroy() - Free a lexer and its internal buffers.
 * @lexer: Lexer to destroy.  Safe to call with NULL.
 */
void lexer_destroy(struct lexer *lexer);

/**
 * lexer_next_token() - Produce the next token from the source stream.
 * @lexer: Active lexer state.
 *
 * The returned token's @value field is heap-allocated and must be
 * freed by the caller (or via token_array_free() in main.c).
 *
 * Return: The next token.  Returns TOKEN_EOF at end of input.
 */
struct token lexer_next_token(struct lexer *lexer);

#endif
