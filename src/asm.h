#ifndef ASM_H
#define ASM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Token types
typedef enum {
    TOKEN_INSTRUCTION,
    TOKEN_REGISTER,
    TOKEN_IMMEDIATE,
    TOKEN_LABEL,           // Label definition (ends with ':')
    TOKEN_LABEL_REFERENCE, // Label reference (used in branch instructions)
    TOKEN_COMMA,
    TOKEN_WORD_DIRECTIVE,  // .word directive
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// BEAG ISA Instruction types
typedef enum {
    INST_ADD,    // 0000
    INST_SUB,    // 0001
    INST_MUL,    // 0010
    INST_DIV,    // 0011
    INST_JALR,   // 0100
    INST_SW,     // 0101
    INST_LW,     // 0110
    INST_LHI,    // 1000
    INST_LLI,    // 1001
    INST_BNE,    // 1101
    INST_BEQ,    // 1110
    INST_BLT,    // 1111
    INST_WORD,   // Word directive
    INST_EOP     // End of program marker
} InstructionType;

// Operand types
typedef enum {
    OP_REGISTER,
    OP_IMMEDIATE,
    OP_LABEL
} OperandType;

// Operand structure
typedef struct {
    OperandType type;
    union {
        uint8_t reg_num;    // 3-bit register number (0-7)
        int immediate;      // 16-bit (or more) immediate for .word
        char* label;        // Label name for branch targets
    } value;
} Operand;

// Instruction structure
typedef struct {
    InstructionType type;
    Operand operands[3];    // Max 3 operands per instruction
    int operand_count;
    int line;
} Instruction;

// Symbol table entry
typedef struct {
    char* name;
    uint16_t value;
    bool is_defined;
} SymbolEntry;

// Function declarations
Token* lexer_init(const char* input);
void lexer_free(Token* tokens);
Instruction* parser_parse(Token* tokens);
void parser_free(Instruction* instructions);
uint16_t* codegen_generate(Instruction* instructions, size_t* size);
void symbol_table_init(void);
void symbol_table_add(const char* name, uint16_t value);
uint16_t symbol_table_get(const char* name);
void symbol_table_free(void);
void debug_print_instructions(Instruction* instructions);
void debug_print_symbol_table(void);
void debug_print_tokens(Token* tokens);

#endif // ASM_H 