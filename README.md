# Python Compiler in C

A basic Python compiler implementation written in C that supports a subset of Python language features including variables, arithmetic operations, control flow, and functions.

## Features

### Supported Python Language Features
- **Variables and Assignment**: `x = 42`
- **Arithmetic Operations**: `+`, `-`, `*`, `/`
- **Comparison Operations**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Control Flow**: `if`/`else` statements, `while` loops
- **Functions**: Function definitions with `def` and function calls
- **Built-in Functions**: `print()` statements
- **Data Types**: Numbers (integers and floats), strings
- **Python-style Indentation**: Proper handling of indented code blocks

### Compiler Architecture
- **Lexical Analysis**: Tokenizes Python source code
- **Parsing**: Builds Abstract Syntax Tree (AST) using recursive descent parser
- **Interpretation**: Tree-walking interpreter for code execution
- **Symbol Tables**: Variable and function scope management

## Project Structure

```
Python-compiler/
├── include/            # Header files
│   ├── compiler.h      # Main type definitions and structures
│   ├── ast.h          # AST node creation functions
│   ├── lexer.h        # Tokenization functions
│   ├── parser.h       # Parsing functions
│   ├── interpreter.h  # Code execution functions
│   ├── symbol_table.h # Variable/function storage
│   └── utils.h        # Utility functions
├── src/               # Source files
│   ├── main.c         # Main compiler driver
│   ├── ast.c          # AST implementation
│   ├── lexer.c        # Lexical analyzer
│   ├── parser.c       # Recursive descent parser
│   ├── interpreter.c  # Tree-walking interpreter
│   ├── symbol_table.c # Symbol table implementation
│   └── utils.c        # File I/O utilities
├── obj/               # Compiled object files
├── Makefile           # Build configuration
└── README.md          # This file
```

## Building the Compiler

### Prerequisites
- C compiler (clang or gcc)
- Make utility

### Compilation
```bash
make
```

This creates the `python-compiler` executable.

### Clean Build
```bash
make clean
make
```

## Usage

### Run Built-in Test Cases
```bash
./python-compiler
```

### Compile and Run Python File
```bash
./python-compiler program.py
```

### Debug Mode (Shows Tokens and AST)
```bash
./python-compiler -d program.py
```

### Help
```bash
./python-compiler --help
```

## Example Programs

### Basic Arithmetic
```python
x = 10
y = 20
result = x + y * 2
print(result)
```

### Conditionals
```python
age = 18
if age >= 18:
    print("Adult")
else:
    print("Minor")
```

### Loops
```python
count = 0
while count < 5:
    print(count)
    count = count + 1
```

### Functions
```python
def square(x):
    return x * x

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

print(square(5))
print(factorial(5))
```

## Implementation Details

### Lexical Analysis
The lexer converts Python source code into tokens, handling:
- Keywords (if, else, while, def, return, print)
- Operators (+, -, *, /, ==, !=, <, >, <=, >=, =)
- Literals (numbers, strings)
- Identifiers (variable and function names)
- Delimiters (parentheses, brackets, colons, commas)
- Python indentation (INDENT/DEDENT tokens)

### Parsing
Recursive descent parser that builds an AST from tokens:
- Expression parsing with operator precedence
- Statement parsing (assignments, control flow, function definitions)
- Block parsing with proper indentation handling
- Error recovery for malformed syntax

### Interpretation
Tree-walking interpreter that executes the AST:
- Variable storage and retrieval
- Function definition and calling with local scopes
- Control flow evaluation
- Built-in function execution (print)
- Runtime error handling

### Symbol Tables
Hierarchical symbol tables for scope management:
- Global scope for top-level variables and functions
- Local scopes for function parameters and variables
- Parent scope lookup for variable resolution

## Limitations

### Not Implemented
- Lists, dictionaries, and other complex data types
- Classes and object-oriented features
- Import statements and modules
- Exception handling (try/except)
- Generators and iterators
- List comprehensions
- Lambda functions
- Multiple assignment (a, b = 1, 2)

### Current Restrictions
- Limited string operations (only concatenation with +)
- No boolean type (uses numbers: 0 = false, non-zero = true)
- Function parameters must match argument count exactly
- No default parameter values
- No variable argument lists

## Development

### Adding New Features
1. Add token types to `TokenType` enum in `compiler.h`
2. Update lexer in `lexer.c` to recognize new syntax
3. Add AST node types to `ASTNodeType` enum
4. Extend parser in `parser.c` for new grammar rules
5. Implement evaluation in `interpreter.c`

### Debugging
Use debug mode to see the compilation process:
```bash
./python-compiler -d program.py
```

This shows:
- Token stream from lexical analysis
- Abstract syntax tree structure
- Execution output

### Code Style
- Follow C99 standard
- Use descriptive variable names
- Include error handling for runtime errors
- Memory management with proper malloc/free usage

## Contributing

When contributing to this project:
1. Maintain the existing code structure
2. Add appropriate error handling
3. Update this README for new features
4. Test with provided example programs
5. Ensure proper memory management

## License

This project is for educational purposes, demonstrating compiler construction techniques and Python language implementation concepts.
