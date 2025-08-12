#include <stdlib.h>
#include <string.h>
#include "ast.h"

// Create a new AST node
ASTNode* ast_create_node(ASTNodeType type, int line_number) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->line_number = line_number;
    return node;
}

// Create number node
ASTNode* ast_create_number(double value, int line) {
    ASTNode* node = ast_create_node(AST_NUMBER, line);
    node->data.number.value = value;
    return node;
}

// Create string node
ASTNode* ast_create_string(char* value, int line) {
    ASTNode* node = ast_create_node(AST_STRING, line);
    node->data.string.value = malloc(strlen(value) + 1);
    strcpy(node->data.string.value, value);
    return node;
}

// Create identifier node
ASTNode* ast_create_identifier(char* name, int line) {
    ASTNode* node = ast_create_node(AST_IDENTIFIER, line);
    node->data.identifier.name = malloc(strlen(name) + 1);
    strcpy(node->data.identifier.name, name);
    return node;
}

// Create binary operation node
ASTNode* ast_create_binary_op(ASTNode* left, TokenType op, ASTNode* right, int line) {
    ASTNode* node = ast_create_node(AST_BINARY_OP, line);
    node->data.binary_op.left = left;
    node->data.binary_op.operator = op;
    node->data.binary_op.right = right;
    return node;
}