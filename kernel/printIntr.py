list=[f"VECTOR 0x{hex(i)[2:].upper()},ZERO" for i in range(0x21, 0x81)]

for vector in list:
    print(vector)