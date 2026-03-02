/*
 * python_compiler.c - Unity build entry point.
 *
 * Includes every translation unit in dependency order so the compiler
 * sees the whole program in one pass.
 *
 * Build:
 *   gcc -Wall -Wextra -std=c99 -O2 -I include/ -o build/python-compiler \
 *       src/python_compiler.c
 */
#include "src/utils.c"
#include "src/ast.c"
#include "src/symbol_table.c"
#include "src/lexer.c"
#include "src/parser.c"
#include "src/interpreter.c"
#include "src/main.c"
