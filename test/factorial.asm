# Factorial calculation program for BEAG ISA
# Input: r1 contains the number to calculate factorial for
# Output: r2 will contain the result

# Initialize result to 1
lli r2, 1      # r2 = 1 (result)
lli r4, 1      # r4 = 1 (constant)
lli r1, 4      # r2 = n

# Check if input is 0
beq r1, done  # if r1 == 0, jump to done

# Initialize counter to input value
add r3, r1, r0 # r3 = r1 (counter)

loop:
    # Multiply result by counter
    mul r2, r2, r3  # r2 = r2 * r3
    
    # Decrement counter
    sub r3, r3, r4  # r3 = r3 - 1
    
    # Check if counter is 0
    beq r3, done   # if r3 == 0, jump to done
    
    # Jump back to loop
    beq r0, loop   # always jump to loop (r0 is always 0)

done:
    # Result is in r2
    # Program ends here 
    beq r0, done
    