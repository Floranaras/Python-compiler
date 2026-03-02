CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2 -I include/
DBFLAGS = -Wall -Wextra -std=c99 -g  -fsanitize=address -I include/

TARGET  = build/python-compiler
UNITY   = python_compiler.c
SRCS    = $(wildcard src/*.c)
HDRS    = $(wildcard include/*.h)

.PHONY: all debug test clean compdb

all: $(TARGET)

$(TARGET): $(UNITY) $(SRCS) $(HDRS) | build/
	$(CC) $(CFLAGS) -o $@ $(UNITY)

debug: $(UNITY) $(SRCS) $(HDRS) | build/
	$(CC) $(DBFLAGS) -o $(TARGET) $(UNITY)

build/:
	mkdir -p build/

test: all
	@passed=0; failed=0; \
	for f in tests/*.py; do \
		$(TARGET) $$f > /dev/null 2>&1 \
		&& passed=$$((passed + 1)) \
		|| failed=$$((failed + 1)); \
	done; \
	echo "$$passed passed, $$failed failed"

#
# compdb - generate compile_commands.json for clangd.
# Run once after cloning, and again when adding source files.
#
compdb:
	@echo '[' > compile_commands.json
	@first=1; \
	for f in $(SRCS); do \
		if [ $$first -eq 0 ]; then printf ',\n' >> compile_commands.json; fi; \
		first=0; \
		printf '  {\n' >> compile_commands.json; \
		printf '    "directory": "%s",\n' "$(PWD)" >> compile_commands.json; \
		printf '    "file": "%s",\n' "$$f" >> compile_commands.json; \
		printf '    "command": "gcc -Wall -Wextra -std=c99 -I include/ -c %s"\n' "$$f" >> compile_commands.json; \
		printf '  }' >> compile_commands.json; \
	done
	@printf '\n]\n' >> compile_commands.json
	@echo "wrote compile_commands.json ($(PWD))"

clean:
	rm -rf build/
