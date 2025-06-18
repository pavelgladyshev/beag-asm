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

// Token type names for debugging
static const char* token_type_names[] = {
    "TOKEN_INSTRUCTION",
    "TOKEN_REGISTER",
    "TOKEN_IMMEDIATE",
    "TOKEN_LABEL",
    "TOKEN_LABEL_REFERENCE",
    "TOKEN_LABEL_HI",
    "TOKEN_LABEL_LO",
    "TOKEN_COMMA",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
    "TOKEN_WORD_DIRECTIVE",
    "TOKEN_ASCII_DIRECTIVE",
    "TOKEN_ASCIZ_DIRECTIVE",
    "TOKEN_STRING_LITERAL",
    "TOKEN_EOF",
    "TOKEN_ERROR"
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
    if (strcmp(str, ".word") == 0) return true;
    if (strcmp(str, ".ascii") == 0) return true;
    if (strcmp(str, ".asciz") == 0) return true;
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
    token->line = line;
    token->column = column;

    // Initialize the appropriate union member based on token type
    switch (type) {
        case TOKEN_REGISTER:
            token->value.reg_num = atoi(value + 1);  // Skip 'r'
            break;
        case TOKEN_IMMEDIATE:
            token->value.immediate = atoi(value);
            break;
        case TOKEN_INSTRUCTION:
        case TOKEN_WORD_DIRECTIVE:
        case TOKEN_ASCII_DIRECTIVE:
        case TOKEN_ASCIZ_DIRECTIVE:
            token->value.inst_type = get_instruction_type(value);
            break;
        case TOKEN_LABEL:
        case TOKEN_LABEL_REFERENCE:
        case TOKEN_STRING_LITERAL:
            token->value.str = strdup(value);
            break;
        default:
            token->value.str = NULL;
    }
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
    return value & 0xFFFF;  // Ensure 16-bit value
}

static char parse_escape_sequence(const char** p, int* column) {
    (*p)++;  // Skip the backslash
    (*column)++;
    
    switch (**p) {
        case 'n':  (*p)++; (*column)++; return '\n';
        case 't':  (*p)++; (*column)++; return '\t';
        case 'r':  (*p)++; (*column)++; return '\r';
        case '\\': (*p)++; (*column)++; return '\\';
        case '\"': (*p)++; (*column)++; return '\"';
        case '0':  (*p)++; (*column)++; return '\0';
        default:
            fprintf(stderr, "Error: Unknown escape sequence '\\%c' at line %d\n", **p, *column);
            return '\\';  // Return backslash as fallback
    }
}

void debug_print_tokens(Token* tokens) {
    printf("\nTokens:\n");
    printf("=======\n");
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        Token* token = &tokens[i];
        printf("%3d: %-12s ", i, token_type_names[token->type]);
        
        // Print the appropriate value based on token type
        switch (token->type) {
            case TOKEN_REGISTER:
                printf("r%d", token->value.reg_num);
                break;
            case TOKEN_IMMEDIATE:
                printf("%d", token->value.immediate);
                break;
            case TOKEN_INSTRUCTION:
            case TOKEN_WORD_DIRECTIVE:
            case TOKEN_ASCII_DIRECTIVE:
            case TOKEN_ASCIZ_DIRECTIVE:
                printf("%s", 
                    token->value.inst_type == INST_ADD ? "add" :
                    token->value.inst_type == INST_SUB ? "sub" :
                    token->value.inst_type == INST_MUL ? "mul" :
                    token->value.inst_type == INST_DIV ? "div" :
                    token->value.inst_type == INST_JALR ? "jalr" :
                    token->value.inst_type == INST_SW ? "sw" :
                    token->value.inst_type == INST_LW ? "lw" :
                    token->value.inst_type == INST_LHI ? "lhi" :
                    token->value.inst_type == INST_LLI ? "lli" :
                    token->value.inst_type == INST_BNE ? "bne" :
                    token->value.inst_type == INST_BEQ ? "beq" :
                    token->value.inst_type == INST_BLT ? "blt" :
                    token->value.inst_type == INST_WORD ? ".word" :
                    token->value.inst_type == INST_ASCII ? ".ascii" :
                    token->value.inst_type == INST_ASCIZ ? ".asciz" : "???");
                break;
            case TOKEN_LABEL:
            case TOKEN_LABEL_REFERENCE:
            case TOKEN_LABEL_HI:
            case TOKEN_LABEL_LO:
            case TOKEN_STRING_LITERAL:
                printf("'%s'", token->value.str);
                break;
            default:
                printf("'%s'", token->value.str ? token->value.str : "");
        }
        
        printf(" at line %d, col %d\n", token->line, token->column);
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

        // Handle string literals
        if (*p == '"') {
            p++;  // Skip opening quote
            column++;
            char* str = malloc(256);  // Temporary buffer for unescaped string
            int str_len = 0;
            
            while (*p && *p != '"') {
                if (*p == '\n') {
                    fprintf(stderr, "Error: Unterminated string literal at line %d\n", line);
                    free(str);
                    lexer_free(tokens);
                    return NULL;
                }
                
                if (*p == '\\') {
                    str[str_len++] = parse_escape_sequence(&p, &column);
                } else {
                    str[str_len++] = *p;
                    p++;
                    column++;
                }
                
                if (str_len >= 255) {  // Leave room for null terminator
                    fprintf(stderr, "Error: String literal too long at line %d\n", line);
                    free(str);
                    lexer_free(tokens);
                    return NULL;
                }
            }
            
            if (!*p) {
                fprintf(stderr, "Error: Unterminated string literal at line %d\n", line);
                free(str);
                lexer_free(tokens);
                return NULL;
            }
            
            str[str_len] = '\0';
            tokens[token_count++] = *create_token(TOKEN_STRING_LITERAL, str, line, column - str_len - 1);
            free(str);
            p++;  // Skip closing quote
            column++;
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
        if (isalpha(*p) || *p == '_' || *p == '.' || *p == '%') {
            const char* start = p;
            while (*p && (isalnum(*p) || *p == '_' || *p == '.' || *p == '%')) {
                p++;
                column++;
            }
            int len = p - start;
            char* ident = malloc(len + 1);
            strncpy(ident, start, len);
            ident[len] = '\0';

            TokenType type;
            if (strcmp(ident, ".word") == 0) {
                type = TOKEN_WORD_DIRECTIVE;
            } else if (strcmp(ident, ".ascii") == 0) {
                type = TOKEN_ASCII_DIRECTIVE;
            } else if (strcmp(ident, ".asciz") == 0) {
                type = TOKEN_ASCIZ_DIRECTIVE;
            } else if (strcmp(ident, "%hi") == 0) {
                type = TOKEN_LABEL_HI;
            } else if (strcmp(ident, "%lo") == 0) {
                type = TOKEN_LABEL_LO;
            } else if (is_instruction(ident)) {
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

            printf("Immediate parsed: %d\n", value);

            // Clamp values
            value = (value) & 0xFFFF;  // For hex, always unsigned


            // Convert the value to a string for debugging
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
            case '(':
                tokens[token_count++] = *create_token(TOKEN_LPAREN, "(", line, column);
                break;
            case ')':
                tokens[token_count++] = *create_token(TOKEN_RPAREN, ")", line, column);
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
        // Only free string values
        if (tokens[i].type == TOKEN_LABEL ||
            tokens[i].type == TOKEN_LABEL_REFERENCE ||
            tokens[i].type == TOKEN_STRING_LITERAL) {
            free(tokens[i].value.str);
        }
    }
    free(tokens);
} 