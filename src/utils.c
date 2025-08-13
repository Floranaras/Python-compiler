#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

// Read entire file into a string
char* read_file(const char* filename) 
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) 
	{
        printf("Error: Could not open file '%s'\n", filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for file content + null terminator
    char* content = malloc(file_size + 1);
    if (content == NULL) 
	{
        printf("Error: Could not allocate memory for file\n");
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';

    fclose(file);
    return content;
}
