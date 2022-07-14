#a = bytearray([0xEA] * 128 * 255)
#a = bytearray([0xEA] * 128 * 256)
a = bytearray([0xEA] * 0x8000)
#a = bytearray([0xEA] * 0x10000)

with open("test.bin", "wb") as f:
    f.write(a)
