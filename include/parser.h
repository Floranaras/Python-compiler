#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"

/**
 * struct parser - Recursive-descent parser state.
 * @tokens:      Token array produced by the lexer (not owned).
 * @position:    Index of the current token.
 * @token_count: Total number of tokens in @tokens.
 */
struct parser {
	struct token	*tokens;
	int		 position;
	int		 token_count;
};

/**
 * parser_create() - Initialise a parser over a token array.
 * @tokens: Token array; must outlive the parser.
 * @count:  Number of tokens in @tokens.
 *
 * Return: Pointer to parser, or NULL on allocation failure.
 */
struct parser *parser_create(struct token *tokens, int count);

/**
 * parser_destroy() - Free the parser struct.
 * @parser: Parser to destroy.  Does not free the token array.
 */
void parser_destroy(struct parser *parser);

/**
 * parser_parse_program() - Parse all top-level statements into an AST.
 * @parser: Initialised parser.
 *
 * Return: AST_PROGRAM root node, or NULL on unrecoverable error.
 *         Caller must call ast_free() on the result.
 */
struct ast_node *parser_parse_program(struct parser *parser);

#endif 
