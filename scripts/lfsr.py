seed = 0xAA
lfsr = seed
print lfsr
period = 0

while (period < 100):
	bit = ((lfsr >> 0) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1
	lfsr = (lfsr >> 1) | (bit << 15)
	print lfsr
	period += 1
