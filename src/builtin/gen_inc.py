#!/usr/bin/env python3
import sys

with open('ast.das', 'rb') as f:
    data = f.read()

with open('ast.das.inc', 'w') as out:
    out.write('static unsigned char ast_das[] = {\n')
    for i in range(0, len(data), 8):
        chunk = data[i:i+8]
        hex_values = ','.join(f'0x{b:02x}' for b in chunk)
        out.write(hex_values + ',\n')
    out.write('};\n')

print(f"Generated ast.das.inc ({len(data)} bytes)")
