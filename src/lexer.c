#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "asm.h"

#define MAX_TOKENS 1024
#define MAX_TOKEN_LENGTH 256

static const char* register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    NULL
};

static const char* instruction_names[] = {
    "add", "sub", "mul", "div",
    "jalr", "sw", "lw",
    "lhi", "lli",
    "bne", "beq", "blt",
    NULL
};

static const char* token_type_names[] = {
    "INSTRUCTION",
    "REGISTER",
    "IMMEDIATE",
    "LABEL",
    "LABEL_REF",
    "COMMA",
    "EOF",
    "ERROR"
};

static bool is_register(const char* str) {
    for (int i = 0; register_names[i] != NULL; i++) {
        if (strcmp(str, register_names[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_instruction(const char* str) {
    for (int i = 0; instruction_names[i] != NULL; i++) {
        if (strcmp(str, instruction_names[i]) == 0) {
            return true;
        }
    }
    return false;
}

static Token* create_token(TokenType type, const char* value, int line, int column) {
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;

    token->type = type;
    token->value = strdup(value);
    token->line = line;
    token->column = column;
    return token;
}

static bool is_hex_digit(char c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int parse_hex(const char* str) {
    int value = 0;
    while (*str) {
        value = value * 16;
        if (isdigit(*str)) {
            value += *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            value += *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            value += *str - 'A' + 10;
        }
        str++;
    }
    return value;
}

void debug_print_tokens(Token* tokens) {
    printf("\nTokens:\n");
    printf("=======\n");
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        Token* token = &tokens[i];
        printf("%3d: %-12s '%s' at line %d, col %d\n",
               i,
               token_type_names[token->type],
               token->value,
               token->line,
               token->column);
    }
    printf("=======\n\n");
}

Token* lexer_init(const char* input) {
    Token* tokens = malloc(sizeof(Token) * MAX_TOKENS);
    if (!tokens) return NULL;

    int token_count = 0;
    int line = 1;
    int column = 1;
    const char* p = input;

    while (*p && token_count < MAX_TOKENS - 1) {
        // Skip whitespace
        while (*p && isspace(*p)) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
        }
        if (!*p) break;

        // Handle comments
        if (*p == '#') {
            while (*p && *p != '\n') {
                p++;
                column++;
            }
            continue;
        }

        // Handle labels (any word ending with ':')
        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            while (*p && (isalnum(*p) || *p == '_')) {
                p++;
                column++;
            }
            
            // Check if this is a label definition (ends with ':')
            if (*p == ':') {
                int len = p - start;
                char* label = malloc(len + 1);
                strncpy(label, start, len);
                label[len] = '\0';
                tokens[token_count++] = *create_token(TOKEN_LABEL, label, line, column - len);
                free(label);
                p++; // Skip the colon
                column++;
                
                // Skip any whitespace after the label
                while (*p && isspace(*p)) {
                    if (*p == '\n') {
                        line++;
                        column = 1;
                    } else {
                        column++;
                    }
                    p++;
                }
                continue;
            }
            
            // If not a label definition, rewind and handle as identifier/instruction
            p = start;
            column = column - (p - start);
        }

        // Handle identifiers and instructions
        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            while (*p && (isalnum(*p) || *p == '_')) {
                p++;
                column++;
            }
            int len = p - start;
            char* ident = malloc(len + 1);
            strncpy(ident, start, len);
            ident[len] = '\0';

            TokenType type;
            if (is_instruction(ident)) {
                type = TOKEN_INSTRUCTION;
            } else if (is_register(ident)) {
                type = TOKEN_REGISTER;
            } else {
                // This is a label reference (used in branch instructions)
                type = TOKEN_LABEL_REFERENCE;
            }

            tokens[token_count++] = *create_token(type, ident, line, column - len);
            free(ident);
            continue;
        }

        // Handle immediate values (decimal and hexadecimal)
        if (isdigit(*p) || *p == '-') {
            const char* start = p;
            int value = 0;

            if (*p == '-') {
                p++;
                column++;
            }

            // Check for hexadecimal format (0x...)
            if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
                p += 2;  // Skip '0x'
                column += 2;
                start = p;  // Reset start to after '0x'

                // Parse hex digits
                while (*p && is_hex_digit(*p)) {
                    p++;
                    column++;
                }

                int len = p - start;
                if (len > 0) {
                    char* hex_str = malloc(len + 1);
                    strncpy(hex_str, start, len);
                    hex_str[len] = '\0';
                    value = parse_hex(hex_str);
                    free(hex_str);
                }
            } else {
                // Parse decimal digits
                while (*p && isdigit(*p)) {
                    p++;
                    column++;
                }
                int len = p - start;
                char* num = malloc(len + 1);
                strncpy(num, start, len);
                num[len] = '\0';
                value = atoi(num);
                free(num);
            }

            // Convert the value to a string
            char value_str[32];
            snprintf(value_str, sizeof(value_str), "%d", value);
            tokens[token_count++] = *create_token(TOKEN_IMMEDIATE, value_str, line, column - (p - start));
            continue;
        }

        // Handle special characters
        switch (*p) {
            case ',':
                tokens[token_count++] = *create_token(TOKEN_COMMA, ",", line, column);
                break;
            default:
                fprintf(stderr, "Error: Unexpected character '%c' at line %d, column %d\n",
                        *p, line, column);
                lexer_free(tokens);
                return NULL;
        }
        p++;
        column++;
    }

    // Add EOF token
    tokens[token_count] = *create_token(TOKEN_EOF, "", line, column);
    
    // Print debug information
    debug_print_tokens(tokens);
    
    return tokens;
}

void lexer_free(Token* tokens) {
    if (!tokens) return;
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
} 