#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

// Forward declarations for recursive descent parsing
static ASTNode* parser_parse_expression(Parser* parser);
static ASTNode* parser_parse_statement(Parser* parser);
static ASTNode* parser_parse_block(Parser* parser);

// Create parser with token array
Parser* parser_create(Token* tokens, int count) {
    Parser* parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->position = 0;
    parser->token_count = count;
    return parser;
}

// Free parser
void parser_destroy(Parser* parser) {
    free(parser);
}

// Get current token
static Token* parser_current_token(Parser* parser) {
    if (parser->position >= parser->token_count) {
        static Token eof_token = {TOKEN_EOF, "EOF", 0, 0, 0};
        return &eof_token;
    }
    return &parser->tokens[parser->position];
}

// Advance to next token
static void parser_advance(Parser* parser) {
    if (parser->position < parser->token_count) {
        parser->position++;
    }
}

// Check if current token matches expected type
static bool parser_match(Parser* parser, TokenType type) {
    return parser_current_token(parser)->type == type;
}

// Consume token if it matches expected type
static bool parser_consume(Parser* parser, TokenType type) {
    if (parser_match(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

// Skip newlines (they're often optional)
static void parser_skip_newlines(Parser* parser) {
    while (parser_match(parser, TOKEN_NEWLINE)) {
        parser_advance(parser);
    }
}

// Parse primary expressions (numbers, identifiers, parentheses)
static ASTNode* parser_parse_primary(Parser* parser) {
    Token* token = parser_current_token(parser);

    // Handle unexpected tokens that shouldn't be in expressions
    if (token->type == TOKEN_ELSE || token->type == TOKEN_COLON ||
        token->type == TOKEN_DEDENT || token->type == TOKEN_EOF) {
        printf("Parse error: unexpected token '%s' at line %d\n", token->value, token->line);
        return NULL;
    }

    if (parser_match(parser, TOKEN_NUMBER)) {
        parser_advance(parser);
        return ast_create_number(token->number, token->line);
    }

    if (parser_match(parser, TOKEN_STRING)) {
        parser_advance(parser);
        return ast_create_string(token->value, token->line);
    }

    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        parser_advance(parser);

        // Check for function call
        if (parser_match(parser, TOKEN_LPAREN)) {
            parser_advance(parser); // consume '('

            ASTNode* call = ast_create_node(AST_FUNCTION_CALL, token->line);
            call->data.function_call.function_name = malloc(strlen(token->value) + 1);
            strcpy(call->data.function_call.function_name, token->value);

            // Parse arguments
            call->data.function_call.arguments = malloc(sizeof(ASTNode*) * 16);
            call->data.function_call.arg_count = 0;

            if (!parser_match(parser, TOKEN_RPAREN)) {
                do {
                    ASTNode* arg = parser_parse_expression(parser);
                    if (arg != NULL) {
                        call->data.function_call.arguments[call->data.function_call.arg_count++] = arg;
                    }
                } while (parser_consume(parser, TOKEN_COMMA));
            }

            parser_consume(parser, TOKEN_RPAREN);
            return call;
        } else {
            return ast_create_identifier(token->value, token->line);
        }
    }

    if (parser_match(parser, TOKEN_LPAREN)) {
        parser_advance(parser); // consume '('
        ASTNode* expr = parser_parse_expression(parser);
        parser_consume(parser, TOKEN_RPAREN);
        return expr;
    }

    // Error: unexpected token
    printf("Parse error: unexpected token '%s' at line %d\n", token->value, token->line);
    return NULL;
}

// Parse unary expressions (-, +)
static ASTNode* parser_parse_unary(Parser* parser) {
    if (parser_match(parser, TOKEN_MINUS) || parser_match(parser, TOKEN_PLUS)) {
        Token* op = parser_current_token(parser);
        parser_advance(parser);

        ASTNode* operand = parser_parse_unary(parser);
        if (operand == NULL) {
            return NULL; // Propagate error
        }

        ASTNode* node = ast_create_node(AST_UNARY_OP, op->line);
        node->data.unary_op.operator = op->type;
        node->data.unary_op.operand = operand;
        return node;
    }

    return parser_parse_primary(parser);
}

// Parse multiplication and division
static ASTNode* parser_parse_term(Parser* parser) {
    ASTNode* left = parser_parse_unary(parser);
    if (left == NULL) {
        return NULL; // Propagate error
    }

    while (parser_match(parser, TOKEN_MULTIPLY) || parser_match(parser, TOKEN_DIVIDE)) {
        Token* op = parser_current_token(parser);
        parser_advance(parser);
        ASTNode* right = parser_parse_unary(parser);
        if (right == NULL) {
            return NULL; // Propagate error
        }
        left = ast_create_binary_op(left, op->type, right, op->line);
    }

    return left;
}

// Parse addition and subtraction
static ASTNode* parser_parse_arithmetic(Parser* parser) {
    ASTNode* left = parser_parse_term(parser);
    if (left == NULL) {
        return NULL; // Propagate error
    }

    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS)) {
        Token* op = parser_current_token(parser);
        parser_advance(parser);
        ASTNode* right = parser_parse_term(parser);
        if (right == NULL) {
            return NULL; // Propagate error
        }
        left = ast_create_binary_op(left, op->type, right, op->line);
    }

    return left;
}

// Parse comparison operations
static ASTNode* parser_parse_comparison(Parser* parser) {
    ASTNode* left = parser_parse_arithmetic(parser);
    if (left == NULL) {
        return NULL; // Propagate error
    }

    while (parser_match(parser, TOKEN_EQUAL) || parser_match(parser, TOKEN_NOT_EQUAL) ||
           parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_GREATER) ||
           parser_match(parser, TOKEN_LESS_EQUAL) || parser_match(parser, TOKEN_GREATER_EQUAL)) {
        Token* op = parser_current_token(parser);
        parser_advance(parser);
        ASTNode* right = parser_parse_arithmetic(parser);
        if (right == NULL) {
            return NULL; // Propagate error
        }
        left = ast_create_binary_op(left, op->type, right, op->line);
    }

    return left;
}

// Parse expressions (top level)
static ASTNode* parser_parse_expression(Parser* parser) {
    return parser_parse_comparison(parser);
}

// Parse if statement
static ASTNode* parser_parse_if_statement(Parser* parser) {
    Token* if_token = parser_current_token(parser);
    parser_advance(parser); // consume 'if'

    ASTNode* condition = parser_parse_expression(parser);
    parser_consume(parser, TOKEN_COLON);
    parser_skip_newlines(parser);

    ASTNode* then_block = parser_parse_block(parser);
    ASTNode* else_block = NULL;

    // Skip any newlines after the block
    parser_skip_newlines(parser);

    // Check for else clause
    if (parser_match(parser, TOKEN_ELSE)) {
        parser_advance(parser); // consume 'else'
        parser_consume(parser, TOKEN_COLON);
        parser_skip_newlines(parser);
        else_block = parser_parse_block(parser);
    }

    ASTNode* node = ast_create_node(AST_IF_STMT, if_token->line);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_block = then_block;
    node->data.if_stmt.else_block = else_block;

    return node;
}

// Parse while statement
static ASTNode* parser_parse_while_statement(Parser* parser) {
    Token* while_token = parser_current_token(parser);
    parser_advance(parser); // consume 'while'

    ASTNode* condition = parser_parse_expression(parser);
    parser_consume(parser, TOKEN_COLON);
    parser_skip_newlines(parser);

    ASTNode* body = parser_parse_block(parser);

    ASTNode* node = ast_create_node(AST_WHILE_STMT, while_token->line);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;

    return node;
}

// Parse function definition
static ASTNode* parser_parse_function_def(Parser* parser) {
    Token* def_token = parser_current_token(parser);
    parser_advance(parser); // consume 'def'

    if (!parser_match(parser, TOKEN_IDENTIFIER)) {
        printf("Parse error: expected function name at line %d\n", def_token->line);
        return NULL;
    }

    Token* name_token = parser_current_token(parser);
    parser_advance(parser);

    parser_consume(parser, TOKEN_LPAREN);

    ASTNode* node = ast_create_node(AST_FUNCTION_DEF, def_token->line);
    node->data.function_def.name = malloc(strlen(name_token->value) + 1);
    strcpy(node->data.function_def.name, name_token->value);

    // Parse parameters
    node->data.function_def.parameters = malloc(sizeof(char*) * 16);
    node->data.function_def.param_count = 0;

    if (!parser_match(parser, TOKEN_RPAREN)) {
        do {
            if (parser_match(parser, TOKEN_IDENTIFIER)) {
                Token* param = parser_current_token(parser);
                parser_advance(parser);

                node->data.function_def.parameters[node->data.function_def.param_count] =
                    malloc(strlen(param->value) + 1);
                strcpy(node->data.function_def.parameters[node->data.function_def.param_count],
                       param->value);
                node->data.function_def.param_count++;
            }
        } while (parser_consume(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RPAREN);
    parser_consume(parser, TOKEN_COLON);
    parser_skip_newlines(parser);

    node->data.function_def.body = parser_parse_block(parser);

    return node;
}

// Parse return statement
static ASTNode* parser_parse_return_statement(Parser* parser) {
    Token* return_token = parser_current_token(parser);
    parser_advance(parser); // consume 'return'

    ASTNode* node = ast_create_node(AST_RETURN_STMT, return_token->line);

    // Check if there's a return value
    if (!parser_match(parser, TOKEN_NEWLINE) && !parser_match(parser, TOKEN_EOF)) {
        node->data.return_stmt.value = parser_parse_expression(parser);
    } else {
        node->data.return_stmt.value = NULL;
    }

    return node;
}

// Parse print statement (built-in function)
static ASTNode* parser_parse_print_statement(Parser* parser) {
    Token* print_token = parser_current_token(parser);
    parser_advance(parser); // consume 'print'

    parser_consume(parser, TOKEN_LPAREN);

    ASTNode* node = ast_create_node(AST_PRINT_STMT, print_token->line);
    node->data.print_stmt.value = parser_parse_expression(parser);

    parser_consume(parser, TOKEN_RPAREN);

    return node;
}

// Parse assignment statement
static ASTNode* parser_parse_assignment(Parser* parser) {
    Token* id_token = parser_current_token(parser);
    parser_advance(parser); // consume identifier

    parser_consume(parser, TOKEN_ASSIGN);

    ASTNode* value = parser_parse_expression(parser);

    ASTNode* node = ast_create_node(AST_ASSIGNMENT, id_token->line);
    node->data.assignment.variable = malloc(strlen(id_token->value) + 1);
    strcpy(node->data.assignment.variable, id_token->value);
    node->data.assignment.value = value;

    return node;
}

// Parse a single statement
static ASTNode* parser_parse_statement(Parser* parser) {
    parser_skip_newlines(parser);

    if (parser_match(parser, TOKEN_EOF) || parser_match(parser, TOKEN_DEDENT)) {
        return NULL;
    }

    Token* token = parser_current_token(parser);

    switch (token->type) {
        case TOKEN_IF:
            return parser_parse_if_statement(parser);
        case TOKEN_WHILE:
            return parser_parse_while_statement(parser);
        case TOKEN_DEF:
            return parser_parse_function_def(parser);
        case TOKEN_RETURN:
            return parser_parse_return_statement(parser);
        case TOKEN_PRINT:
            return parser_parse_print_statement(parser);
        case TOKEN_IDENTIFIER:
            // Look ahead to see if it's an assignment
            if (parser->position + 1 < parser->token_count &&
                parser->tokens[parser->position + 1].type == TOKEN_ASSIGN) {
                return parser_parse_assignment(parser);
            } else {
                // It's an expression statement
                ASTNode* expr = parser_parse_expression(parser);
                return expr;
            }
        case TOKEN_ELSE:
        case TOKEN_COLON:
        case TOKEN_DEDENT:
            // These tokens should not appear at statement level
            printf("Parse error: unexpected token '%s' at line %d (context error)\n",
                   token->value, token->line);
            parser_advance(parser); // Skip the problematic token and continue
            return NULL;
        default: {
            // Try to parse as expression, but handle errors gracefully
            ASTNode* expr = parser_parse_expression(parser);
            if (expr == NULL) {
                // If expression parsing fails, skip the token and continue
                printf("Skipping unparseable token '%s' at line %d\n", token->value, token->line);
                parser_advance(parser);
                return NULL;
            }
            return expr;
        }
    }
}

// Parse a block of statements (indented)
static ASTNode* parser_parse_block(Parser* parser) {
    ASTNode* block = ast_create_node(AST_BLOCK, parser_current_token(parser)->line);
    block->data.block.statements = malloc(sizeof(ASTNode*) * 64);
    block->data.block.count = 0;

    if (!parser_consume(parser, TOKEN_INDENT)) {
        // If no indent token, create empty block
        return block;
    }

    while (!parser_match(parser, TOKEN_DEDENT) && !parser_match(parser, TOKEN_EOF)) {
        ASTNode* stmt = parser_parse_statement(parser);
        if (stmt != NULL) {
            block->data.block.statements[block->data.block.count++] = stmt;
        }
        parser_skip_newlines(parser);

        // Safety check to prevent infinite loops
        if (block->data.block.count >= 64) {
            printf("Error: Block too large (over 64 statements)\n");
            break;
        }
    }

    parser_consume(parser, TOKEN_DEDENT);

    return block;
}

// Parse entire program
ASTNode* parser_parse_program(Parser* parser) {
    ASTNode* program = ast_create_node(AST_PROGRAM, 1);
    program->data.program.statements = malloc(sizeof(ASTNode*) * 256);
    program->data.program.count = 0;

    while (!parser_match(parser, TOKEN_EOF)) {
        ASTNode* stmt = parser_parse_statement(parser);
        if (stmt != NULL) {
            program->data.program.statements[program->data.program.count++] = stmt;
        }
        parser_skip_newlines(parser);
    }

    return program;
}