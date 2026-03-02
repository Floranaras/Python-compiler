#include "utils.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_INIT_CAP	1024

/* --- Token array --------------------------------------------------------- */

static void token_array_free(struct token *tokens, int count)
{
	int j;

	if (!tokens)
		return;
	for (j = 0; j < count; j++)
		free(tokens[j].value);
	free(tokens);
}

/* grow_tokens() - Double token array capacity. Returns new pointer or NULL. */
static struct token *grow_tokens(struct token *tokens, int *capacity)
{
	struct token	*grown;
	int		 new_cap = (*capacity) * 2;

	grown = realloc(tokens, sizeof(struct token) * new_cap);
	if (!grown)
		return NULL;
	*capacity = new_cap;
	return grown;
}

static struct token *tokenise(const char *source, int *count)
{
	struct lexer *lex;
	struct token *tokens;
	struct token *grown;
	int capacity = TOKEN_INIT_CAP;
	struct token tok;

	if (!source || !count)
		return NULL;

	lex = lexer_create(source);
	if (!lex)
		return NULL;

	tokens = malloc(sizeof(struct token) * capacity);
	if (!tokens) {
		lexer_destroy(lex);
		return NULL;
	}

	*count = 0;

	for (;;) {
		tok = lexer_next_token(lex);

		if (*count >= capacity) {
			grown = grow_tokens(tokens, &capacity);
			if (!grown) {
				token_array_free(tokens, *count);
				lexer_destroy(lex);
				return NULL;
			}
			tokens = grown;
		}

		tokens[(*count)++] = tok;

		if (tok.type == TOKEN_EOF || tok.type == TOKEN_ERROR)
			break;
	}

	lexer_destroy(lex);
	return tokens;
}

/* --- Compilation pipeline ----------------------------------------------- */

static int compile_and_run(const char *source)
{
	int token_count = 0;
	struct token *tokens;
	struct parser *parser;
	struct ast_node	*ast;
	struct interpreter *interp;
	int j;
	int rc = 0;

	if (!source)
		return 1;

	tokens = tokenise(source, &token_count);
	if (!tokens)
		return 1;

	for (j = 0; j < token_count; j++) {
		if (tokens[j].type != TOKEN_ERROR)
			continue;
		fprintf(stderr,
			"lexer error: invalid token '%s' "
			"at line %d\n",
			tokens[j].value, tokens[j].line);
		token_array_free(tokens, token_count);
		return 1;
	}

	parser = parser_create(tokens, token_count);
	if (!parser) {
		token_array_free(tokens, token_count);
		return 1;
	}

	ast = parser_parse_program(parser);
	parser_destroy(parser);

	if (!ast) {
		fprintf(stderr, "parse error: could not build AST\n");
		token_array_free(tokens, token_count);
		return 1;
	}

	interp = interpreter_create();
	if (!interp) {
		rc = 1;
		goto done;
	}

	interpreter_evaluate(interp, ast);
	interpreter_destroy(interp);

done:
	ast_free(ast);
	token_array_free(tokens, token_count);
	return rc;
}

/* --- Built-in tests ------------------------------------------------------ */

static void run_tests(void)
{
	static const struct {
		const char *name;
		const char *source;
	} tests[] = {
		{
			"arithmetic",
			"x = 10\n"
			"y = 20\n"
			"result = x + y * 2\n"
			"print(result)\n"
		},
		{
			"conditionals",
			"age = 18\n"
			"if age >= 18:\n"
			"    print(\"Adult\")\n"
		},
		{
			"while loop",
			"count = 0\n"
			"while count < 3:\n"
			"    print(count)\n"
			"    count = count + 1\n"
		},
		{
			"functions",
			"def square(x):\n"
			"    return x * x\n"
			"\n"
			"result = square(5)\n"
			"print(result)\n"
		},
		{
			"recursion",
			"def factorial(n):\n"
			"    if n <= 1:\n"
			"        return 1\n"
			"    else:\n"
			"        return n * factorial(n - 1)\n"
			"\n"
			"print(factorial(5))\n"
		},
		{
			"comments",
			"# compute a product\n"
			"x = 6\n"
			"y = 7\n"
			"print(x * y)\n"
		},
		{
			"string concat",
			"a = \"hello\"\n"
			"b = \" world\"\n"
			"print(a + b)\n"
		},
	};

	int ntests = (int)(sizeof(tests) / sizeof(tests[0]));
	int j;

	printf("Running %d built-in tests\n\n", ntests);
	for (j = 0; j < ntests; j++) {
		printf("--- Test %d: %s ---\n", j + 1, tests[j].name);
		compile_and_run(tests[j].source);
		printf("\n");
	}
}

/* --- Entry point --------------------------------------------------------- */

int main(int argc, char *argv[])
{
	char	*source;
	int	 rc;

	if (argc == 1) {
		run_tests();
		return 0;
	}

	if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		printf("Usage: %s [file.py]\n", argv[0]);
		return 0;
	}

	if (argc != 2) {
		fprintf(stderr,
			"error: expected one argument\n"
			"Usage: %s [file.py]\n", argv[0]);
		return 1;
	}

	source = read_file(argv[1]);
	if (!source)
		return 1;

	rc = compile_and_run(source);
	free(source);
	return rc;
}
