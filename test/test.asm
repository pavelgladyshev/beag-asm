loop: lli r2, 0x1      # r2 = 1 (result)
lhi r2, 0x1F     # Load high byte
beq r0,loop