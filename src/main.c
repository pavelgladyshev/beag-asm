#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"

#define MAX_FILE_SIZE 4096

static char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }

    char* buffer = malloc(MAX_FILE_SIZE);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t size = fread(buffer, 1, MAX_FILE_SIZE - 1, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

static void write_file(const char* filename, uint16_t* code, size_t size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s' for writing\n", filename);
        return;
    }

    fwrite(code, sizeof(uint16_t), size, file);
    fclose(file);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.bin>\n", argv[0]);
        return 1;
    }

    // Read input file
    char* input = read_file(argv[1]);
    if (!input) return 1;

    // Initialize symbol table
    symbol_table_init();

    // Lexical analysis
    Token* tokens = lexer_init(input);
    if (!tokens) {
        free(input);
        return 1;
    }

    // Parsing
    Instruction* instructions = parser_parse(tokens);
    if (!instructions) {
        lexer_free(tokens);
        free(input);
        return 1;
    }

    // Code generation
    size_t code_size;
    uint16_t* code = codegen_generate(instructions, &code_size);
    if (!code) {
        parser_free(instructions);
        lexer_free(tokens);
        free(input);
        return 1;
    }

    // Write output file
    write_file(argv[2], code, code_size);

    // Cleanup
    free(code);
    parser_free(instructions);
    lexer_free(tokens);
    free(input);
    symbol_table_free();

    return 0;
} 