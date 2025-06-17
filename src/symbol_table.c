#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"

#define MAX_SYMBOLS 1024

typedef struct {
    SymbolEntry entries[MAX_SYMBOLS];
    int count;
} SymbolTable;

static SymbolTable symbol_table;

void symbol_table_init(void) {
    symbol_table.count = 0;
}

void symbol_table_add(const char* name, uint16_t value) {
    // Check if symbol already exists
    for (int i = 0; i < symbol_table.count; i++) {
        if (strcmp(symbol_table.entries[i].name, name) == 0) {
            // If symbol is already defined, this is an error
            if (symbol_table.entries[i].is_defined) {
                fprintf(stderr, "Error: Symbol '%s' redefined\n", name);
                return;
            }
            // Update existing undefined symbol
            symbol_table.entries[i].value = value;
            symbol_table.entries[i].is_defined = true;
            return;
        }
    }

    // Add new symbol
    if (symbol_table.count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Symbol table full\n");
        return;
    }

    SymbolEntry* entry = &symbol_table.entries[symbol_table.count++];
    entry->name = strdup(name);
    entry->value = value;
    entry->is_defined = true;
}

uint16_t symbol_table_get(const char* name) {
    for (int i = 0; i < symbol_table.count; i++) {
        if (strcmp(symbol_table.entries[i].name, name) == 0) {
            return symbol_table.entries[i].value;
        }
    }

    // Symbol not found, add as undefined
    if (symbol_table.count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Symbol table full\n");
        return 0xFFFF;
    }

    SymbolEntry* entry = &symbol_table.entries[symbol_table.count++];
    entry->name = strdup(name);
    entry->value = 0;
    entry->is_defined = false;
    return 0xFFFF;
}

void symbol_table_free(void) {
    for (int i = 0; i < symbol_table.count; i++) {
        free(symbol_table.entries[i].name);
    }
    symbol_table.count = 0;
}

void debug_print_symbol_table(void) {
    printf("\nSymbol Table:\n");
    printf("============\n");
    for (int i = 0; i < symbol_table.count; i++) {
        printf("%-20s -> %d (%s)\n", 
               symbol_table.entries[i].name,
               symbol_table.entries[i].value,
               symbol_table.entries[i].is_defined ? "defined" : "undefined");
    }
    printf("============\n\n");
} 