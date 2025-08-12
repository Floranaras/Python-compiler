#ifndef TOKEN_H
#define TOKEN_H

// =============================================================================
// TOKEN DEFINITIONS
// =============================================================================

typedef enum {
    // Literals
    TOKEN_NUMBER,       // 42, 3.14
    TOKEN_STRING,       // "hello"
    TOKEN_IDENTIFIER,   // variable_name

    // Operators
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_MULTIPLY,     // *
    TOKEN_DIVIDE,       // /
    TOKEN_ASSIGN,       // =
    TOKEN_EQUAL,        // ==
    TOKEN_NOT_EQUAL,    // !=
    TOKEN_LESS,         // <
    TOKEN_GREATER,      // >
    TOKEN_LESS_EQUAL,   // <=
    TOKEN_GREATER_EQUAL,// >=

    // Delimiters
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    TOKEN_COMMA,        // ,
    TOKEN_COLON,        // :

    // Keywords
    TOKEN_IF,           // if
    TOKEN_ELSE,         // else
    TOKEN_WHILE,        // while
    TOKEN_DEF,          // def
    TOKEN_RETURN,       // return
    TOKEN_PRINT,        // print (built-in)

    // Special
    TOKEN_NEWLINE,      // \n
    TOKEN_INDENT,       // Indentation increase
    TOKEN_DEDENT,       // Indentation decrease
    TOKEN_EOF,          // End of file
    TOKEN_ERROR         // Lexical error
} TokenType;

typedef struct {
    TokenType type;
    char* value;        // Token's string value
    int line;           // Line number
    int column;         // Column number
    double number;      // For numeric tokens
} Token;

#endif // TOKEN_H