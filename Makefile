# Compiler to use
CC = gcc

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Compiler flags:
# -Wall:    Enable all warnings
# -g:       Include debugging information
# -std=c99: Use the C99 standard
# -I<dir>:  Tell compiler where to find header files (.h files)
CFLAGS = -Wall -g -std=c99 -I$(INC_DIR)

# The name of the final executable
TARGET = python-compiler.exe

# List all your .c files without the path
SOURCES = \
    main.c \
    lexer.c \
    parser.c \
    ast.c \
    symbol_table.c \
    interpreter.c \
    utils.c

# Create the list of object files by replacing .c with .o and adding the obj/ path
OBJS = $(addprefix $(OBJ_DIR)/, $(SOURCES:.c=.o))

# The default rule, executed when you just type "make"
# It builds the final executable.
all: $(TARGET)

# Rule to link all the object files into the final executable
$(TARGET): $(OBJS)
	@echo "Linking executable..."
	$(CC) $(CFLAGS) -o $@ $^

# This rule creates the obj/ directory before compiling anything.
# The | $(OBJ_DIR) part makes it an "order-only prerequisite".
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the object directory.
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Rule to clean up the project directory
# Removes the executable and the entire obj directory.
clean:
	@echo "Cleaning up project files..."
	rm -rf $(OBJ_DIR) $(TARGET)

# A convenience rule to compile and then immediately run the built-in tests
run: all
	./$(TARGET)

# A convenience rule to compile and run a specific Python file
runfile: all
	@# Example: make runfile FILE=test.py
	./$(TARGET) $(FILE)

# Declares that 'all', 'clean', 'run', and 'runfile' are not actual files
.PHONY: all clean run runfile