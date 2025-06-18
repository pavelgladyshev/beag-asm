#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"

#define MAX_INSTRUCTIONS 1024

static Instruction* instructions = NULL;
static int instruction_count = 0;
static Token* current_token = NULL;

static void parse_error(const char* message) {
    fprintf(stderr, "Error at line %d: %s\n", current_token->line, message);
}

static void advance(void) {
    current_token++;
}

static bool match(TokenType type) {
    if (current_token->type == type) {
        advance();
        return true;
    }
    return false;
}

static InstructionType get_instruction_type(const char* name) {
    if (strcmp(name, "add") == 0) return INST_ADD;
    if (strcmp(name, "sub") == 0) return INST_SUB;
    if (strcmp(name, "mul") == 0) return INST_MUL;
    if (strcmp(name, "div") == 0) return INST_DIV;
    if (strcmp(name, "jalr") == 0) return INST_JALR;
    if (strcmp(name, "sw") == 0) return INST_SW;
    if (strcmp(name, "lw") == 0) return INST_LW;
    if (strcmp(name, "lhi") == 0) return INST_LHI;
    if (strcmp(name, "lli") == 0) return INST_LLI;
    if (strcmp(name, "bne") == 0) return INST_BNE;
    if (strcmp(name, "beq") == 0) return INST_BEQ;
    if (strcmp(name, "blt") == 0) return INST_BLT;
    return INST_EOP;
}

static void parse_register(Operand* operand) {
    if (current_token->type != TOKEN_REGISTER) {
        parse_error("Expected register");
        return;
    }
    operand->type = OP_REGISTER;
    operand->value.reg_num = atoi(current_token->value + 1);  // Skip 'r'
    advance();
}

static void parse_immediate(Operand* operand) {
    if (current_token->type != TOKEN_IMMEDIATE) {
        parse_error("Expected immediate value");
        return;
    }
    operand->type = OP_IMMEDIATE;
    operand->value.immediate = atoi(current_token->value);
    printf("'%s' Immediate parsed: %d\n", current_token->value, operand->value.immediate);
    advance();
}

static void parse_label(Operand* operand) {
    if (current_token->type != TOKEN_LABEL_REFERENCE) {
        parse_error("Expected label reference");
        return;
    }
    operand->type = OP_LABEL;
    operand->value.label = strdup(current_token->value);
    advance();
}

static void parse_operands(Instruction* inst) {
    inst->operand_count = 0;
    
    // Parse first operand
    if (current_token->type == TOKEN_REGISTER) {
        parse_register(&inst->operands[inst->operand_count++]);
    } else if (current_token->type == TOKEN_IMMEDIATE) {
        parse_immediate(&inst->operands[inst->operand_count++]);
    } else if (current_token->type == TOKEN_LABEL_REFERENCE) {
        parse_label(&inst->operands[inst->operand_count++]);
    }

    // Parse additional operands
    while (match(TOKEN_COMMA) && inst->operand_count < 3) {
        if (current_token->type == TOKEN_REGISTER) {
            parse_register(&inst->operands[inst->operand_count++]);
        } else if (current_token->type == TOKEN_IMMEDIATE) {
            parse_immediate(&inst->operands[inst->operand_count++]);
        } else if (current_token->type == TOKEN_LABEL_REFERENCE) {
            parse_label(&inst->operands[inst->operand_count++]);
        } else {
            parse_error("Expected register, immediate, or label");
            break;
        }
    }
}

static void parse_instruction(void) {
    if (current_token->type != TOKEN_INSTRUCTION) {
        parse_error("Expected instruction");
        return;
    }

    Instruction* inst = &instructions[instruction_count++];
    inst->type = get_instruction_type(current_token->value);
    inst->line = current_token->line;
    advance();

    parse_operands(inst);
}

static void parse_label_definition(void) {
    if (current_token->type != TOKEN_LABEL) {
        parse_error("Expected label definition");
        return;
    }

    // Add label to symbol table with current instruction count
    symbol_table_add(current_token->value, instruction_count);
    advance();
}

static void parse_word_directive(void) {
    if (current_token->type != TOKEN_WORD_DIRECTIVE) {
        parse_error("Expected .word directive");
        return;
    }

    Instruction* inst = &instructions[instruction_count++];
    inst->type = INST_WORD;
    inst->line = current_token->line;
    advance();

    // Parse the word value (number or label)
    if (current_token->type == TOKEN_IMMEDIATE) {
        parse_immediate(&inst->operands[0]);
        inst->operand_count = 1;
    } else if (current_token->type == TOKEN_LABEL_REFERENCE) {
        parse_label(&inst->operands[0]);
        inst->operand_count = 1;
    } else {
        parse_error("Expected number or label after .word");
    }
}

Instruction* parser_parse(Token* tokens) {
    instructions = malloc(sizeof(Instruction) * MAX_INSTRUCTIONS);
    if (!instructions) return NULL;

    instruction_count = 0;
    current_token = tokens;

    while (current_token->type != TOKEN_EOF && instruction_count < MAX_INSTRUCTIONS) {
        if (current_token->type == TOKEN_LABEL) {
            parse_label_definition();
        } else if (current_token->type == TOKEN_INSTRUCTION) {
            parse_instruction();
        } else if (current_token->type == TOKEN_WORD_DIRECTIVE) {
            parse_word_directive();
        } else {
            parse_error("Unexpected token");
            break;
        }
    }

    // Add end of program marker
    instructions[instruction_count].type = INST_EOP;
    instructions[instruction_count].operand_count = 0;
    instructions[instruction_count].line = current_token->line;

    // Print debug information
    debug_print_instructions(instructions);
    debug_print_symbol_table();

    return instructions;
}

void parser_free(Instruction* instructions) {
    if (!instructions) return;
    
    // Free any label strings in operands
    for (int i = 0; instructions[i].type != INST_EOP; i++) {
        for (int j = 0; j < instructions[i].operand_count; j++) {
            if (instructions[i].operands[j].type == OP_LABEL) {
                free(instructions[i].operands[j].value.label);
            }
        }
    }
    
    free(instructions);
}

void debug_print_instructions(Instruction* instructions) {
    printf("\nParsed Instructions:\n");
    printf("===================\n");
    
    for (int i = 0; instructions[i].type != INST_EOP; i++) {
        Instruction* inst = &instructions[i];
        printf("%3d: %-8s ", i, 
               inst->type == INST_ADD ? "add" :
               inst->type == INST_SUB ? "sub" :
               inst->type == INST_MUL ? "mul" :
               inst->type == INST_DIV ? "div" :
               inst->type == INST_JALR ? "jalr" :
               inst->type == INST_SW ? "sw" :
               inst->type == INST_LW ? "lw" :
               inst->type == INST_LHI ? "lhi" :
               inst->type == INST_LLI ? "lli" :
               inst->type == INST_BNE ? "bne" :
               inst->type == INST_BEQ ? "beq" :
               inst->type == INST_BLT ? "blt" :
               inst->type == INST_WORD ? ".word" : "???");

        // Print operands
        for (int j = 0; j < inst->operand_count; j++) {
            if (j > 0) printf(", ");
            switch (inst->operands[j].type) {
                case OP_REGISTER:
                    printf("r%d", inst->operands[j].value.reg_num);
                    break;
                case OP_IMMEDIATE:
                    printf("%d (0x%04X)", 
                           inst->operands[j].value.immediate,
                           (int16_t)inst->operands[j].value.immediate);
                    break;
                case OP_LABEL:
                    printf("%s", inst->operands[j].value.label);
                    break;
            }
        }

        // Print instruction format type
        printf(" [%s] (line %d)\n",
               inst->type == INST_WORD ? "Word" :
               inst->type <= INST_DIV ? "R-type" :
               inst->type <= INST_LW ? "M-type" : "I-type",
               inst->line);
    }
    printf("===================\n\n");
} 