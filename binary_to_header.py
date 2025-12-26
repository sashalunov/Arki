# This script reads binary files and converts them into C++ header file format.

import os

def file_to_hex(filename, array_name):
    with open(filename, "rb") as f:
        data = f.read()
    
    out = [f"0x{b:02x}" for b in data]
    
    # Format as C++ array
    content = f"const unsigned char {array_name}[{len(data)}] = {{\n"
    content += ", ".join(out)
    content += "\n};\n"
    content += f"const int {array_name}_SIZE = {len(data)};\n\n"
    return content

# Convert your specific files
header_content = "#pragma once\n\n"
header_content += file_to_hex("xolo.fnt", "FONT_DATA")
header_content += file_to_hex("xolo_0.png", "FONT_TEXTURE")

with open("EmbeddedAssets.h", "w") as f:
    f.write(header_content)

print("Generated EmbeddedAssets.h!")
