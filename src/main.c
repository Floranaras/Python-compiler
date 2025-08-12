#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "utils.h"

// =============================================================================
// MAIN COMPILER DRIVER
// =============================================================================

// Tokenize source code
static Token* tokenize(char* source, int* token_count) {
    Lexer* lexer = lexer_create(source);
    Token* tokens = malloc(sizeof(Token) * 1024);
    *token_count = 0;
    int capacity = 1024;

    while (true) {
        Token token = lexer_next_token(lexer);

        // Reallocate if we need more space
        if (*token_count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, sizeof(Token) * capacity);
        }

        tokens[*token_count] = token;
        (*token_count)++;

        if (token.type == TOKEN_EOF || token.type == TOKEN_ERROR) {
            break;
        }
    }

    lexer_destroy(lexer);
    return tokens;
}

// Print tokens (for debugging)
static void print_tokens(Token* tokens, int count) {
    printf("=== TOKENS ===\n");
    for (int i = 0; i < count; i++) {
        printf("Line %d: %s (%d)\n", tokens[i].line, tokens[i].value, tokens[i].type);
    }
    printf("\n");
}

// Print AST (for debugging)
static void print_ast(ASTNode* node, int indent) {
    if (node == NULL) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %g\n", node->data.number.value);
            break;
        case AST_STRING:
            printf("String: \"%s\"\n", node->data.string.value);
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        case AST_BINARY_OP:
            printf("BinaryOp: %d\n", node->data.binary_op.operator);
            print_ast(node->data.binary_op.left, indent + 1);
            print_ast(node->data.binary_op.right, indent + 1);
            break;
        case AST_ASSIGNMENT:
            printf("Assignment: %s =\n", node->data.assignment.variable);
            print_ast(node->data.assignment.value, indent + 1);
            break;
        case AST_IF_STMT:
            printf("If:\n");
            print_ast(node->data.if_stmt.condition, indent + 1);
            print_ast(node->data.if_stmt.then_block, indent + 1);
            if (node->data.if_stmt.else_block) {
                print_ast(node->data.if_stmt.else_block, indent + 1);
            }
            break;
        case AST_PRINT_STMT:
            printf("Print:\n");
            print_ast(node->data.print_stmt.value, indent + 1);
            break;
        case AST_BLOCK:
            printf("Block:\n");
            for (int i = 0; i < node->data.block.count; i++) {
                print_ast(node->data.block.statements[i], indent + 1);
            }
            break;
        case AST_PROGRAM:
            printf("Program:\n");
            for (int i = 0; i < node->data.program.count; i++) {
                print_ast(node->data.program.statements[i], indent + 1);
            }
            break;
        default:
            printf("Unknown node type\n");
    }
}

// Compile and run Python source code
void compile_and_run(char* source, bool debug) {
    printf("=== COMPILING PYTHON CODE ===\n");

    // Step 1: Tokenization
    int token_count;
    Token* tokens = tokenize(source, &token_count);

    if (debug) {
        print_tokens(tokens, token_count);
    }

    // Check for tokenization errors
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_ERROR) {
            printf("Tokenization error: invalid token '%s' at line %d\n",
                   tokens[i].value, tokens[i].line);
            return;
        }
    }

    // Step 2: Parsing
    Parser* parser = parser_create(tokens, token_count);
    ASTNode* ast = parser_parse_program(parser);

    if (ast == NULL) {
        printf("Parsing failed\n");
        parser_destroy(parser);
        free(tokens);
        return;
    }

    if (debug) {
        printf("=== AST ===\n");
        print_ast(ast, 0);
        printf("\n");
    }

    // Step 3: Interpretation/Execution
    printf("=== EXECUTION OUTPUT ===\n");
    Interpreter* interpreter = interpreter_create();
    interpreter_evaluate(interpreter, ast);

    // Cleanup
    parser_destroy(parser);
    free(tokens);
    free(interpreter);

    printf("\n=== COMPILATION COMPLETE ===\n");
}

// =============================================================================
// EXAMPLE USAGE AND TESTS
// =============================================================================

static void show_usage(const char* program_name) {
    printf("Basic Python Compiler in C\n");
    printf("===========================\n\n");
    printf("Usage:\n");
    printf("  %s                    - Run built-in test cases\n", program_name);
    printf("  %s <file.py>          - Compile and run Python file\n", program_name);
    printf("  %s -d <file.py>       - Compile with debug output\n", program_name);
    printf("  %s --help             - Show this help message\n", program_name);
    printf("\nSupported Python features:\n");
    printf("  • Variables and assignment: x = 42\n");
    printf("  • Arithmetic: +, -, *, /\n");
    printf("  • Comparisons: ==, !=, <, >, <=, >=\n");
    printf("  • Control flow: if/else, while loops\n");
    printf("  • Functions: def func(params): ...\n");
    printf("  • Built-in: print()\n");
    printf("  • String literals: \"hello\"\n");
    printf("  • Proper Python indentation\n");
}

static void run_builtin_tests() {
    printf("Running built-in test cases...\n\n");

    // Test 1: Basic arithmetic
    printf("Test 1: Basic Arithmetic\n");
    char* test1 =
        "x = 10\n"
        "y = 20\n"
        "result = x + y * 2\n"
        "print(result)\n";
    compile_and_run(test1, false);

    // Test 2: Conditionals
    printf("\nTest 2: Conditionals\n");
    char* test2 =
        "age = 18\n"
        "if age >= 18:\n"
        "    print(\"Adult\")\n";
    compile_and_run(test2, false);

    // Test 3: Loops
    printf("\nTest 3: While Loop\n");
    char* test3 =
        "count = 0\n"
        "while count < 3:\n"
        "    print(count)\n"
        "    count = count + 1\n";
    compile_and_run(test3, false);

    // Test 4: Functions
    printf("\nTest 4: Functions\n");
    char* test4 =
        "def square(x):\n"
        "    return x * x\n"
        "\n"
        "result = square(5)\n"
        "print(result)\n";
    compile_and_run(test4, false);

    // Test 5: Factorial function
    printf("\nTest 5: Factorial Function\n");
    char* test5 =
        "def factorial(n):\n"
        "    if n <= 1:\n"
        "        return 1\n"
        "    else:\n"
        "        return n * factorial(n - 1)\n"
        "\n"
        "print(factorial(5))\n";
    compile_and_run(test5, false);
}

int main(int argc, char* argv[]) {
    // No arguments - run built-in tests
    if (argc == 1) {
        run_builtin_tests();
        return 0;
    }

    // Help option
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        show_usage(argv[0]);
        return 0;
    }

    // Debug mode with file
    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        char* source = read_file(argv[2]);
        if (source == NULL) {
            return 1;
        }

        printf("Compiling '%s' with debug output:\n", argv[2]);
        printf("================================\n");
        compile_and_run(source, true);  // Enable debug output

        free(source);
        return 0;
    }

    // Regular file compilation
    if (argc == 2) {
        char* source = read_file(argv[1]);
        if (source == NULL) {
            return 1;
        }

        printf("Compiling and running '%s':\n", argv[1]);
        printf("============================\n");
        compile_and_run(source, false);

        free(source);
        return 0;
    }

    // Invalid arguments
    printf("Error: Invalid arguments\n\n");
    show_usage(argv[0]);
    return 1;
}
