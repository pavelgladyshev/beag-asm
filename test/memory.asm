# Memory operations test program for BEAG ISA
# This program demonstrates loading and storing values in memory

# Load immediate values into registers
lli r1, 42     # r1 = 42
lli r2, 100    # r2 = 100

# Store r1 to memory address in r2
sw r1, r2      # [r2] = r1

# Load the value back into r3
lw r3, r2      # r3 = [r2]

# Now r3 should contain 42
# Program ends here 