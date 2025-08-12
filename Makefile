# Cross-platform Makefile for Python Compiler

# Detect the operating system
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

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

# Platform-specific settings
ifeq ($(DETECTED_OS),Windows)
    TARGET = python-compiler.exe
    MKDIR = mkdir
    RM = del /Q /S
    RMDIR = rmdir /Q /S
    PATH_SEP = \\
    RUN_PREFIX = 
else
    TARGET = python-compiler
    MKDIR = mkdir -p
    RM = rm -f
    RMDIR = rm -rf
    PATH_SEP = /
    RUN_PREFIX = ./
endif

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
OBJS = $(addprefix $(OBJ_DIR)$(PATH_SEP), $(SOURCES:.c=.o))

# The default rule, executed when you just type "make"
# It builds the final executable.
all: $(TARGET)

# Rule to link all the object files into the final executable
$(TARGET): $(OBJS)
	@echo "Linking executable..."
	$(CC) $(CFLAGS) -o $@ $^

# Platform-specific compilation rules
ifeq ($(DETECTED_OS),Windows)
$(OBJ_DIR)$(PATH_SEP)%.o: $(SRC_DIR)$(PATH_SEP)%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@
else
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@
endif

# Rule to create the object directory
$(OBJ_DIR):
ifeq ($(DETECTED_OS),Windows)
	@if not exist $(OBJ_DIR) $(MKDIR) $(OBJ_DIR)
else
	@$(MKDIR) $(OBJ_DIR)
endif

# Rule to clean up the project directory
clean:
	@echo "Cleaning up project files..."
ifeq ($(DETECTED_OS),Windows)
	@if exist $(OBJ_DIR) $(RMDIR) $(OBJ_DIR)
	@if exist $(TARGET) $(RM) $(TARGET)
else
	@$(RMDIR) $(OBJ_DIR) 2>/dev/null || true
	@$(RM) $(TARGET)
endif

# A convenience rule to compile and then immediately run the built-in tests
run: all
	@echo "Running $(TARGET)..."
	$(RUN_PREFIX)$(TARGET)

# A convenience rule to compile and run a specific Python file
runfile: all
	@echo "Running $(TARGET) with file: $(FILE)"
ifndef FILE
	@echo "Usage: make runfile FILE=yourfile.py"
else
	$(RUN_PREFIX)$(TARGET) $(FILE)
endif

# Display detected OS and build info
info:
	@echo "Detected OS: $(DETECTED_OS)"
	@echo "Target: $(TARGET)"
	@echo "Compiler: $(CC)"
	@echo "Source files: $(SOURCES)"

# Install dependencies (example - adjust as needed)
install-deps:
ifeq ($(DETECTED_OS),Windows)
	@echo "On Windows, ensure MinGW-w64 or similar is installed"
	@echo "You may need to install make for Windows (e.g., via chocolatey: choco install make)"
else ifeq ($(DETECTED_OS),Darwin)
	@echo "On macOS, ensure Xcode command line tools are installed:"
	@echo "xcode-select --install"
else
	@echo "On Linux, ensure build-essential is installed:"
	@echo "sudo apt-get install build-essential  # Ubuntu/Debian"
	@echo "sudo yum groupinstall 'Development Tools'  # RHEL/CentOS"
endif

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the executable (default)"
	@echo "  clean     - Remove built files"
	@echo "  run       - Build and run the executable"
	@echo "  runfile   - Build and run with a file (make runfile FILE=test.py)"
	@echo "  info      - Show build information"
	@echo "  install-deps - Show dependency installation instructions"
	@echo "  help      - Show this help message"

# Declares that these are not actual files
.PHONY: all clean run runfile info install-deps help
