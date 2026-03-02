#ifndef TOKEN_H
#define TOKEN_H

/**
 * enum token_type - All token types produced by the lexer
 *
 * Covers literals, operators delimiters, keywordsm and 
 * the structural INDENT/DEDENT tokens the encode Python's
 * significant-whitespace grammar
 */
enum token_type {
	/* Literals */
	TOKEN_NUMBER,		/* 42, 3.14             */
	TOKEN_STRING,		/* "hello"              */
	TOKEN_IDENTIFIER,	/* variable_name        */

	/* Arithmetic */
	TOKEN_PLUS,		/* +                    */
	TOKEN_MINUS,		/* -                    */
	TOKEN_MULTIPLY,		/* *                    */
	TOKEN_DIVIDE,		/* /                    */

	/* Assignment and comparison */
	TOKEN_ASSIGN,		/* =                    */
	TOKEN_EQUAL,		/* ==                   */
	TOKEN_NOT_EQUAL,	/* !=                   */
	TOKEN_LESS,		/* <                    */
	TOKEN_GREATER,		/* >                    */
	TOKEN_LESS_EQUAL,	/* <=                   */
	TOKEN_GREATER_EQUAL,	/* >=                   */

	/* Delimiters */
	TOKEN_LPAREN,		/* (                    */
	TOKEN_RPAREN,		/* )                    */
	TOKEN_COMMA,		/* ,                    */
	TOKEN_COLON,		/* :                    */

	/* Keywords */
	TOKEN_IF,		/* if                   */
	TOKEN_ELSE,		/* else                 */
	TOKEN_WHILE,		/* while                */
	TOKEN_DEF,		/* def                  */
	TOKEN_RETURN,		/* return               */
	TOKEN_PRINT,		/* print (built-in)     */

	/* Whitespace structure */
	TOKEN_NEWLINE,		/* \n                   */
	TOKEN_INDENT,		/* indentation increase */
	TOKEN_DEDENT,		/* indentation decrease */

	/* Sentinels */
	TOKEN_EOF,		/* end of input         */
	TOKEN_ERROR		/* unrecognised char    */
};

/**
 * struct token - A single lexical token
 * @type: Classification of this token
 * @value: Heap-allocated string form; caller must free()
 * @line: Source line (1 - based)
 * @column: Source column (1 - based)
 * @number: Numeric value; only valid when type == TOKEN_NUMBER
 */
struct token {
	enum token_type type;
	char *value;
	int line;
	int column;
	double number;
};

#endif 
