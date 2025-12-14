import struct

# Command values based on the enum and example
# 4=DOWN, 1=LOAD, 204800=WAIT(8s), 2=RELEASE, 3=UP
commands = [4, 1, 204800, 2, 3]

# Write as 32-bit unsigned integers (Little Endian)
with open('marble_commands.bin', 'wb') as f:
    for cmd in commands:
        f.write(struct.pack('<I', cmd))

print("marble_commands.bin created successfully.")
