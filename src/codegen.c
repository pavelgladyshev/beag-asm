#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"

#define MAX_CODE_SIZE 65536  // 2^16 instructions max

uint16_t* codegen_generate(Instruction* instructions, size_t* size) {
    if (!instructions || !size) return NULL;

    // First pass: count instructions and resolve labels
    // Note: BEAG uses word-addressable memory (16-bit words)
    // Each instruction takes exactly one memory word
    int instruction_count = 0;
    int current_address = 0;  // Address in terms of 16-bit words
    for (int i = 0; instructions[i].type != INST_EOP; i++) {
        instruction_count++;
        current_address++;  // Each instruction takes one memory word
    }

    // Allocate memory for the machine code
    // Each instruction is 16 bits (one memory word)
    uint16_t* machine_code = malloc(instruction_count * sizeof(uint16_t));
    if (!machine_code) return NULL;

    // Second pass: generate machine code
    current_address = 0;  // Reset address counter for second pass
    for (int i = 0; instructions[i].type != INST_EOP; i++) {
        Instruction* inst = &instructions[i];
        uint16_t instruction = 0;

        switch (inst->type) {
            case INST_ADD:
                instruction = (0x0 << 12) |  // opcode [15:12]
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |  // rd [10:8]
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |  // rs1 [6:4]
                            (inst->operands[2].value.reg_num & 0x7);          // rs2 [2:0]
                break;

            case INST_SUB:
                instruction = (0x1 << 12) |
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |
                            (inst->operands[2].value.reg_num & 0x7);
                break;

            case INST_MUL:
                instruction = (0x2 << 12) |
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |
                            (inst->operands[2].value.reg_num & 0x7);
                break;

            case INST_DIV:
                instruction = (0x3 << 12) |
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |
                            (inst->operands[2].value.reg_num & 0x7);
                break;

            case INST_JALR:
                instruction = (0x4 << 12) |
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |
                            (inst->operands[2].value.reg_num & 0x7);
                break;

            case INST_SW:
                instruction = (0x5 << 12) |  // opcode [15:12]
                            0x0 |            // [11:8] = 0000 (unused)
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |  // ra [6:4]
                            (inst->operands[0].value.reg_num & 0x7);          // rs [2:0]
                break;

            case INST_LW:
                instruction = (0x6 << 12) |  // opcode [15:12]
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |  // rd [10:8]
                            ((inst->operands[1].value.reg_num & 0x7) << 4) |  // ra [6:4]
                            0x0;                                              // [3:0] = 0000 (unused)
                break;

            case INST_LHI:
                instruction = (0x8 << 12) |  // opcode [15:12]
                            0x0 |            // [11] = 0 (unused)
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |  // rd [10:8]
                            0x0 |            // [7] = 0 (unused)
                            (inst->operands[1].value.immediate & 0xFF);       // imm8 [7:0]
                break;

            case INST_LLI:
                instruction = (0x9 << 12) |  // opcode [15:12]
                            0x0 |            // [11] = 0 (unused)
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |  // rd [10:8]
                            0x0 |            // [7] = 0 (unused)
                            (inst->operands[1].value.immediate & 0xFF);       // imm8 [7:0]
                break;

            case INST_BNE:
            case INST_BEQ:
            case INST_BLT: {
                uint16_t opcode;
                switch (inst->type) {
                    case INST_BNE: opcode = 0xD; break;  // Branch if register != 0
                    case INST_BEQ: opcode = 0xE; break;  // Branch if register == 0
                    case INST_BLT: opcode = 0xF; break;  // Branch if register < 0
                    default: opcode = 0;  // Should never happen
                }
                
                // Get target address from symbol table
                // Note: All addresses in symbol table are in terms of 16-bit words
                uint16_t target = symbol_table_get(inst->operands[1].value.label);
                if (target == 0xFFFF) {
                    fprintf(stderr, "Error: Undefined label '%s' at line %d\n",
                            inst->operands[1].value.label, inst->line);
                    free(machine_code);
                    return NULL;
                }
                
                // Calculate branch offset
                // BEAG uses word-addressable memory, so:
                // - current_address is in terms of 16-bit words
                // - target is also in terms of 16-bit words
                // For branch instructions:
                // - PC is not pre-incremented (unlike regular instructions)
                // - offset is relative to current instruction's address
                // - positive offset means jump forward
                // - negative offset means jump backward
                // - 8-bit signed offset range: -128 to +127 instructions
                //   This means we can branch up to 127 instructions forward
                //   or 128 instructions backward from current position
                // Example:
                //   0x0000: beq r1, target    ; current_address = 0
                //   0x0001: add r2, r3, r4    ; skipped if branch taken
                //   0x0002: target: sub r5, r6, r7
                //   offset = 2 - 0 = 2 (jump forward by 2 instructions)
                //
                // Branch conditions:
                // - BEQ: branch if register value == 0
                // - BNE: branch if register value != 0
                // - BLT: branch if register value < 0
                int16_t offset = target - current_address;
                if (offset < -128 || offset > 127) {  // 8-bit signed offset
                    fprintf(stderr, "Error: Branch target too far at line %d\n", inst->line);
                    free(machine_code);
                    return NULL;
                }
                
                instruction = (opcode << 12) |  // opcode [15:12]
                            0x0 |              // [11] = 0 (unused)
                            ((inst->operands[0].value.reg_num & 0x7) << 8) |  // rd [10:8] - register to check
                            0x0 |              // [7] = 0 (unused)
                            (offset & 0xFF);                                   // imm8 [7:0] - branch offset
                break;
            }

            default:
                fprintf(stderr, "Error: Unknown instruction type at line %d\n", inst->line);
                free(machine_code);
                return NULL;
        }

        // Store instruction in memory
        // Each instruction takes exactly one 16-bit word
        machine_code[current_address] = instruction;
        current_address++;
    }

    *size = instruction_count;
    return machine_code;
} 