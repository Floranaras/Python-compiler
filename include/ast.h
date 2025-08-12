#ifndef AST_H
#define AST_H

#include "token.h"

// =============================================================================
// ABSTRACT SYNTAX TREE (AST) DEFINITIONS
// =============================================================================

typedef enum {
    AST_PROGRAM,        // Root node
    AST_NUMBER,         // Numeric literal
    AST_STRING,         // String literal
    AST_IDENTIFIER,     // Variable reference
    AST_BINARY_OP,      // Binary operation (+, -, *, /, ==, etc.)
    AST_UNARY_OP,       // Unary operation (-, +)
    AST_ASSIGNMENT,     // Variable assignment
    AST_IF_STMT,        // If statement
    AST_WHILE_STMT,     // While loop
    AST_FUNCTION_DEF,   // Function definition
    AST_FUNCTION_CALL,  // Function call
    AST_RETURN_STMT,    // Return statement
    AST_PRINT_STMT,     // Print statement (built-in)
    AST_BLOCK           // Block of statements
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    int line_number;

    union {
        // Literals
        struct {
            double value;
        } number;

        struct {
            char* value;
        } string;

        struct {
            char* name;
        } identifier;

        // Operations
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
            TokenType operator;
        } binary_op;

        struct {
            struct ASTNode* operand;
            TokenType operator;
        } unary_op;

        // Statements
        struct {
            char* variable;
            struct ASTNode* value;
        } assignment;

        struct {
            struct ASTNode* condition;
            struct ASTNode* then_block;
            struct ASTNode* else_block;  // Can be NULL
        } if_stmt;

        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_stmt;

        struct {
            char* name;
            char** parameters;      // Array of parameter names
            int param_count;
            struct ASTNode* body;
        } function_def;

        struct {
            char* function_name;
            struct ASTNode** arguments;
            int arg_count;
        } function_call;

        struct {
            struct ASTNode* value;  // Can be NULL for bare return
        } return_stmt;

        struct {
            struct ASTNode* value;
        } print_stmt;

        // Collections
        struct {
            struct ASTNode** statements;
            int count;
        } block;

        struct {
            struct ASTNode** statements;
            int count;
        } program;

    } data;
} ASTNode;

// Create a new AST node
ASTNode* ast_create_node(ASTNodeType type, int line_number);

// Create number node
ASTNode* ast_create_number(double value, int line);

// Create string node
ASTNode* ast_create_string(char* value, int line);

// Create identifier node
ASTNode* ast_create_identifier(char* name, int line);

// Create binary operation node
ASTNode* ast_create_binary_op(ASTNode* left, TokenType op, ASTNode* right, int line);

#endif // AST_H