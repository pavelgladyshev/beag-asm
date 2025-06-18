#!/bin/bash

# Build the assembler using the top-level Makefile
cd ..
make
cd test

# Assemble the test programs and print their binary output
for asm in factorial.asm memory.asm test.asm word.asm; do
    bin_file="${asm%.asm}.bin"
    echo "Assembling $asm -> $bin_file"
    ../bin/beag-asm "$asm" "$bin_file"
    echo "Hexdump of $bin_file:"
    hexdump "$bin_file"
    echo
    echo "-----------------------------"
done

echo "Build and test complete!" 