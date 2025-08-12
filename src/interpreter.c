#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "interpreter.h"

// Forward declaration
Value interpreter_evaluate(Interpreter* interp, ASTNode* node);

// Evaluate binary operations
static Value evaluate_binary_op(Interpreter* interp, ASTNode* node) {
    Value left = interpreter_evaluate(interp, node->data.binary_op.left);
    Value right = interpreter_evaluate(interp, node->data.binary_op.right);
    Value result;

    // Arithmetic operations
    if (left.type == VALUE_NUMBER && right.type == VALUE_NUMBER) {
        result.type = VALUE_NUMBER;

        switch (node->data.binary_op.operator) {
            case TOKEN_PLUS:
                result.data.number = left.data.number + right.data.number;
                break;
            case TOKEN_MINUS:
                result.data.number = left.data.number - right.data.number;
                break;
            case TOKEN_MULTIPLY:
                result.data.number = left.data.number * right.data.number;
                break;
            case TOKEN_DIVIDE:
                if (right.data.number == 0) {
                    printf("Runtime error: Division by zero at line %d\n", node->line_number);
                    result.data.number = 0;
                } else {
                    result.data.number = left.data.number / right.data.number;
                }
                break;
            case TOKEN_EQUAL:
                result.data.number = (left.data.number == right.data.number) ? 1 : 0;
                break;
            case TOKEN_NOT_EQUAL:
                result.data.number = (left.data.number != right.data.number) ? 1 : 0;
                break;
            case TOKEN_LESS:
                result.data.number = (left.data.number < right.data.number) ? 1 : 0;
                break;
            case TOKEN_GREATER:
                result.data.number = (left.data.number > right.data.number) ? 1 : 0;
                break;
            case TOKEN_LESS_EQUAL:
                result.data.number = (left.data.number <= right.data.number) ? 1 : 0;
                break;
            case TOKEN_GREATER_EQUAL:
                result.data.number = (left.data.number >= right.data.number) ? 1 : 0;
                break;
            default:
                printf("Runtime error: Unknown binary operator at line %d\n", node->line_number);
                result.data.number = 0;
        }
    }
    // String concatenation
    else if (left.type == VALUE_STRING && right.type == VALUE_STRING &&
             node->data.binary_op.operator == TOKEN_PLUS) {
        result.type = VALUE_STRING;
        int len = strlen(left.data.string) + strlen(right.data.string) + 1;
        result.data.string = malloc(len);
        strcpy(result.data.string, left.data.string);
        strcat(result.data.string, right.data.string);
    }
    else {
        printf("Runtime error: Type mismatch in binary operation at line %d\n", node->line_number);
        result.type = VALUE_NONE;
    }

    return result;
}

// Evaluate unary operations
static Value evaluate_unary_op(Interpreter* interp, ASTNode* node) {
    Value operand = interpreter_evaluate(interp, node->data.unary_op.operand);
    Value result;

    if (operand.type == VALUE_NUMBER) {
        result.type = VALUE_NUMBER;

        switch (node->data.unary_op.operator) {
            case TOKEN_MINUS:
                result.data.number = -operand.data.number;
                break;
            case TOKEN_PLUS:
                result.data.number = operand.data.number;
                break;
            default:
                printf("Runtime error: Unknown unary operator at line %d\n", node->line_number);
                result.data.number = 0;
        }
    } else {
        printf("Runtime error: Cannot apply unary operator to non-number at line %d\n",
               node->line_number);
        result.type = VALUE_NONE;
    }

    return result;
}

// Evaluate function call
static Value evaluate_function_call(Interpreter* interp, ASTNode* node) {
    Symbol* func_symbol = symbol_table_find(interp->current_scope,
                                           node->data.function_call.function_name);

    if (func_symbol == NULL || func_symbol->value.type != VALUE_FUNCTION) {
        printf("Runtime error: Undefined function '%s' at line %d\n",
               node->data.function_call.function_name, node->line_number);
        Value result;
        result.type = VALUE_NONE;
        return result;
    }

    ASTNode* func_def = func_symbol->value.data.function;

    // Create new scope for function execution
    SymbolTable* func_scope = symbol_table_create(interp->current_scope);
    SymbolTable* old_scope = interp->current_scope;
    interp->current_scope = func_scope;

    // Bind parameters to arguments
    for (int i = 0; i < func_def->data.function_def.param_count; i++) {
        if (i < node->data.function_call.arg_count) {
            Value arg_value = interpreter_evaluate(interp, node->data.function_call.arguments[i]);
            symbol_table_set(func_scope, func_def->data.function_def.parameters[i], arg_value);
        }
    }

    // Execute function body
    bool old_returned = interp->has_returned;
    Value old_return_value = interp->return_value;
    interp->has_returned = false;

    interpreter_evaluate(interp, func_def->data.function_def.body);

    Value result = interp->return_value;

    // Restore previous state
    interp->current_scope = old_scope;
    interp->has_returned = old_returned;
    interp->return_value = old_return_value;

    // Clean up function scope
    free(func_scope->symbols);
    free(func_scope);

    return result;
}

// Create interpreter
Interpreter* interpreter_create() {
    Interpreter* interp = malloc(sizeof(Interpreter));
    interp->global_scope = symbol_table_create(NULL);
    interp->current_scope = interp->global_scope;
    interp->has_returned = false;
    interp->return_value.type = VALUE_NONE;
    return interp;
}

// Main evaluation function
Value interpreter_evaluate(Interpreter* interp, ASTNode* node) {
    Value result;
    result.type = VALUE_NONE;

    if (node == NULL) {
        return result;
    }

    switch (node->type) {
        case AST_NUMBER:
            result.type = VALUE_NUMBER;
            result.data.number = node->data.number.value;
            break;

        case AST_STRING:
            result.type = VALUE_STRING;
            result.data.string = malloc(strlen(node->data.string.value) + 1);
            strcpy(result.data.string, node->data.string.value);
            break;

        case AST_IDENTIFIER: {
            Symbol* symbol = symbol_table_find(interp->current_scope,
                                             node->data.identifier.name);
            if (symbol != NULL) {
                result = symbol->value;
            } else {
                printf("Runtime error: Undefined variable '%s' at line %d\n",
                       node->data.identifier.name, node->line_number);
            }
            break;
        }

        case AST_BINARY_OP:
            result = evaluate_binary_op(interp, node);
            break;

        case AST_UNARY_OP:
            result = evaluate_unary_op(interp, node);
            break;

        case AST_ASSIGNMENT: {
            Value value = interpreter_evaluate(interp, node->data.assignment.value);
            symbol_table_set(interp->current_scope, node->data.assignment.variable, value);
            result = value;
            break;
        }

        case AST_IF_STMT: {
            Value condition = interpreter_evaluate(interp, node->data.if_stmt.condition);
            bool is_true = (condition.type == VALUE_NUMBER && condition.data.number != 0);

            if (is_true) {
                result = interpreter_evaluate(interp, node->data.if_stmt.then_block);
            } else if (node->data.if_stmt.else_block != NULL) {
                result = interpreter_evaluate(interp, node->data.if_stmt.else_block);
            }
            break;
        }

        case AST_WHILE_STMT: {
            while (true) {
                Value condition = interpreter_evaluate(interp, node->data.while_stmt.condition);
                bool is_true = (condition.type == VALUE_NUMBER && condition.data.number != 0);

                if (!is_true || interp->has_returned) {
                    break;
                }

                interpreter_evaluate(interp, node->data.while_stmt.body);
            }
            break;
        }

        case AST_FUNCTION_DEF: {
            Value func_value;
            func_value.type = VALUE_FUNCTION;
            func_value.data.function = node;
            symbol_table_set(interp->current_scope, node->data.function_def.name, func_value);
            break;
        }

        case AST_FUNCTION_CALL:
            result = evaluate_function_call(interp, node);
            break;

        case AST_RETURN_STMT:
            if (node->data.return_stmt.value != NULL) {
                interp->return_value = interpreter_evaluate(interp, node->data.return_stmt.value);
            } else {
                interp->return_value.type = VALUE_NONE;
            }
            interp->has_returned = true;
            break;

        case AST_PRINT_STMT: {
            Value value = interpreter_evaluate(interp, node->data.print_stmt.value);

            switch (value.type) {
                case VALUE_NUMBER:
                    if (value.data.number == (int)value.data.number) {
                        printf("%.0f\n", value.data.number);
                    } else {
                        printf("%g\n", value.data.number);
                    }
                    break;
                case VALUE_STRING:
                    printf("%s\n", value.data.string);
                    break;
                case VALUE_NONE:
                    printf("None\n");
                    break;
                default:
                    printf("<unknown>\n");
            }
            break;
        }

        case AST_BLOCK:
            for (int i = 0; i < node->data.block.count && !interp->has_returned; i++) {
                result = interpreter_evaluate(interp, node->data.block.statements[i]);
            }
            break;

        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.count; i++) {
                result = interpreter_evaluate(interp, node->data.program.statements[i]);
            }
            break;

        default:
            printf("Runtime error: Unknown AST node type at line %d\n", node->line_number);
    }

    return result;
}