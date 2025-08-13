#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "lexer.h"

// Initialize the lexer with source code
Lexer* lexer_create(char* source) 
{
    Lexer* lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;

    // Initialize indent stack with capacity of 32 levels
    lexer->indent_capacity = 32;
    lexer->indent_stack = malloc(sizeof(int) * lexer->indent_capacity);
    lexer->indent_stack[0] = 0;  // Base indentation level
    lexer->indent_top = 0;
    lexer->at_line_start = true;

    // Add fields to track pending dedents
    lexer->pending_dedents = 0;

    return lexer;
}

// Free lexer memory
void lexer_destroy(Lexer* lexer) 
{
    free(lexer->indent_stack);
    free(lexer);
}

// Get current character without advancing
static char lexer_peek(Lexer* lexer) 
{
    if (lexer->position >= strlen(lexer->source)) 
        return '\0';

    return lexer->source[lexer->position];
}

// Get current character and advance position
static char lexer_advance(Lexer* lexer) 
{
    if (lexer->position >= strlen(lexer->source)) 
        return '\0';

    char c = lexer->source[lexer->position++];

    if (c == '\n') 
	{
        lexer->line++;
        lexer->column = 1;
        lexer->at_line_start = true;
    } 
	else 
	{
        lexer->column++;
        if (c != ' ' && c != '\t') 
            lexer->at_line_start = false;
    }

    return c;
}

// Skip whitespace (but not newlines, they're significant in Python)
static void lexer_skip_whitespace(Lexer* lexer) 
{
    while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t') 
        lexer_advance(lexer);
}

// Check if string matches keyword
static TokenType check_keyword(char* str) 
{
    if (!strcmp(str, "if")) return TOKEN_IF;
    if (!strcmp(str, "else")) return TOKEN_ELSE;
    if (!strcmp(str, "while")) return TOKEN_WHILE;
    if (!strcmp(str, "def")) return TOKEN_DEF;
    if (!strcmp(str, "return")) return TOKEN_RETURN;
    if (!strcmp(str, "print")) return TOKEN_PRINT;
    return TOKEN_IDENTIFIER;
}

// Read a number (integer or float)
static Token lexer_read_number(Lexer* lexer) 
{
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;

    char buffer[256];
    int j = 0;
    bool has_dot = false;

    // Read digits and optional decimal point
    while (isdigit(lexer_peek(lexer)) || (lexer_peek(lexer) == '.' && !has_dot)) 
	{
        if (lexer_peek(lexer) == '.') 
            has_dot = true;
        buffer[j++] = lexer_advance(lexer);
	}

    buffer[j] = '\0';
    token.type = TOKEN_NUMBER;
    token.value = malloc(strlen(buffer) + 1);
    strcpy(token.value, buffer);
    token.number = atof(buffer);

    return token;
}

// Read an identifier or keyword
static Token lexer_read_identifier(Lexer* lexer) 
{
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;

    char buffer[256];
    int j = 0;

    // Read alphanumeric characters and underscores
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_') 
        buffer[j++] = lexer_advance(lexer);

    buffer[j] = '\0';
    token.value = malloc(strlen(buffer) + 1);
    strcpy(token.value, buffer);
    token.type = check_keyword(buffer);

    return token;
}

// Read a string literal
static Token lexer_read_string(Lexer* lexer) 
{
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOKEN_STRING;

    char quote = lexer_advance(lexer); // Skip opening quote
    char buffer[1024];
    int j = 0;

    while (lexer_peek(lexer) != quote && lexer_peek(lexer) != '\0') 
	{
        if (lexer_peek(lexer) == '\\') 
		{
            lexer_advance(lexer); // Skip backslash
            char escaped = lexer_advance(lexer);
            switch (escaped) 
			{
                case 'n': buffer[j++] = '\n'; break;
                case 't': buffer[j++] = '\t'; break;
                case 'r': buffer[j++] = '\r'; break;
                case '\\': buffer[j++] = '\\'; break;
                case '"': buffer[j++] = '"'; break;
                case '\'': buffer[j++] = '\''; break;
                default: buffer[j++] = escaped; break;
            }
        } 

		else 	
            buffer[j++] = lexer_advance(lexer);
    }

    if (lexer_peek(lexer) == quote) 
        lexer_advance(lexer); // Skip closing quote

    buffer[j] = '\0';
    token.value = malloc(strlen(buffer) + 1);
    strcpy(token.value, buffer);

    return token;
}

// Get the next token from the source
Token lexer_next_token(Lexer* lexer) 
{
    // First, check if we have pending DEDENT tokens to return
    if (lexer->pending_dedents > 0) {
        lexer->pending_dedents--;
        Token token;
        token.type = TOKEN_DEDENT;
        token.value = malloc(8);
        strcpy(token.value, "DEDENT");
        token.line = lexer->line;
        token.column = 1;
        return token;
    }

    // Handle indentation at the start of lines
    if (lexer->at_line_start) 
	{
        // Check if this line has any content (not just whitespace/newline)
        int temp_pos = lexer->position;
        int spaces = 0;

        // Count leading spaces/tabs
        while (temp_pos < strlen(lexer->source) &&
               (lexer->source[temp_pos] == ' ' || 
				lexer->source[temp_pos] == '\t')) 
		{
            if (lexer->source[temp_pos] == '\t') 
                spaces += 4;
            else 
                spaces += 1;

            temp_pos++;
        }

        // If this line is not empty (not just newline or EOF)
        if (temp_pos < strlen(lexer->source) &&
            lexer->source[temp_pos] != '\n' &&
            lexer->source[temp_pos] != '\0') 
		{

            int current_indent = lexer->indent_stack[lexer->indent_top];

            if (spaces > current_indent) 
			{
                // Consume the whitespace
                while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t') 
                    lexer_advance(lexer);

                lexer->at_line_start = false;

                // Indentation increased - push new level
                lexer->indent_top++;
                lexer->indent_stack[lexer->indent_top] = spaces;

                Token token;
                token.type = TOKEN_INDENT;
                token.value = malloc(8);
                strcpy(token.value, "INDENT");
                token.line = lexer->line;
                token.column = 1;
                return token;
            } 
			else if (spaces < current_indent) 
			{
                // Consume the whitespace
                while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
                    lexer_advance(lexer);

                lexer->at_line_start = false;

                // Indentation decreased - count how many levels we need to pop
                int dedent_count = 0;
                while (lexer->indent_top > 0 
					&& lexer->indent_stack[lexer->indent_top] > spaces) {
                    lexer->indent_top--;
                    dedent_count++;
                }

                // Set pending dedents (we'll return one now, the rest later)
                lexer->pending_dedents = dedent_count - 1;

                Token token;
                token.type = TOKEN_DEDENT;
                token.value = malloc(8);
                strcpy(token.value, "DEDENT");
                token.line = lexer->line;
                token.column = 1;
                return token;
            }
        }
    }

    lexer_skip_whitespace(lexer);

    Token token;
    token.line = lexer->line;
    token.column = lexer->column;

    char c = lexer_peek(lexer);

    // End of file - generate any pending DEDENT tokens
    if (c == '\0') 
	{
        if (lexer->indent_top > 0) 
		{
            lexer->indent_top--;
            token.type = TOKEN_DEDENT;
            token.value = malloc(8);
            strcpy(token.value, "DEDENT");
            return token;
        } 
		else 
		{
            token.type = TOKEN_EOF;
            token.value = malloc(4);
            strcpy(token.value, "EOF");
            return token;
        }
    }

    // Numbers
    if (isdigit(c)) 
        return lexer_read_number(lexer);

    // Identifiers and keywords
    if (isalpha(c) || c == '_') 
        return lexer_read_identifier(lexer);

    // String literals
    if (c == '"' || c == '\'') 
        return lexer_read_string(lexer);

    // Single character tokens
    lexer_advance(lexer);
    token.value = malloc(2);
    token.value[1] = '\0';

    switch (c) 
	{
        case '+': token.type = TOKEN_PLUS; token.value[0] = '+'; break;
        case '-': token.type = TOKEN_MINUS; token.value[0] = '-'; break;
        case '*': token.type = TOKEN_MULTIPLY; token.value[0] = '*'; break;
        case '/': token.type = TOKEN_DIVIDE; token.value[0] = '/'; break;
        case '(': token.type = TOKEN_LPAREN; token.value[0] = '('; break;
        case ')': token.type = TOKEN_RPAREN; token.value[0] = ')'; break;
        case '[': token.type = TOKEN_LBRACKET; token.value[0] = '['; break;
        case ']': token.type = TOKEN_RBRACKET; token.value[0] = ']'; break;
        case ',': token.type = TOKEN_COMMA; token.value[0] = ','; break;
        case ':': token.type = TOKEN_COLON; token.value[0] = ':'; break;
        case '\n':
            token.type = TOKEN_NEWLINE;
            token.value[0] = '\n';
            lexer->at_line_start = true;
            break;
        case '=':
            if (lexer_peek(lexer) == '=') 
			{
                lexer_advance(lexer);
                token.type = TOKEN_EQUAL;
                free(token.value);
                token.value = malloc(3);
                strcpy(token.value, "==");
            } 
			else 
			{
                token.type = TOKEN_ASSIGN;
                token.value[0] = '=';
            }
            break;
        case '!':
            if (lexer_peek(lexer) == '=') 
			{
                lexer_advance(lexer);
                token.type = TOKEN_NOT_EQUAL;
                free(token.value);
                token.value = malloc(3);
                strcpy(token.value, "!=");
            } 
			else 
			{
                token.type = TOKEN_ERROR;
                token.value[0] = '!';
            }
            break;
        case '<':
            if (lexer_peek(lexer) == '=') 
			{
                lexer_advance(lexer);
                token.type = TOKEN_LESS_EQUAL;
                free(token.value);
                token.value = malloc(3);
                strcpy(token.value, "<=");
            } 
			else 
			{
                token.type = TOKEN_LESS;
                token.value[0] = '<';
            }
            break;
        case '>':
            if (lexer_peek(lexer) == '=') 
			{
                lexer_advance(lexer);
                token.type = TOKEN_GREATER_EQUAL;
                free(token.value);
                token.value = malloc(3);
                strcpy(token.value, ">=");
            } 
			else 
			{
                token.type = TOKEN_GREATER;
                token.value[0] = '>';
            }
            break;
        default:
            token.type = TOKEN_ERROR;
            token.value[0] = c;
            break;
    }

    return token;
}
