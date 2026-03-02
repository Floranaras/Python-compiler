/*
 * Request POSIX.1-2008 extensions before any system header is pulled in.
 * Defining it here covers every translation unit in both the unity build
 * (python_compiler.c includes this first) and traditional separate builds
 * (each .c includes this as its first header).
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE	200809L
#endif

#ifndef UTILS_H
#define UTILS_H

/**
 * read_file() - Read a file's contents into a heap-allocated string.
 * @filename: Path to the file.
 *
 * Return: NUL-terminated buffer on success; caller must free().
 *         NULL on error (message printed to stderr).
 */
char *read_file(const char *filename);

#endif /* UTILS_H */
