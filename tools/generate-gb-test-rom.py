#!/usr/bin/env python3
"""Create rights-clear, original Game Boy DMG striped-screen acceptance ROM."""
from argparse import ArgumentParser
from pathlib import Path
def generate():
    rom=bytearray(32768)
    rom[0x100:0x103]=bytes([0xC3,0x50,0x01])
    rom[0x134:0x142]=b"SUITE DMG TEST"
    rom[0x143]=0;rom[0x147]=0;rom[0x148]=0;rom[0x149]=0
    program=bytes([
        0x3E,0x00,0xE0,0x40,0x21,0x00,0x80,0x06,0x08,
        0x3E,0xAA,0x22,0xAF,0x22,0x05,0x20,0xF8,
        0x3E,0xE4,0xE0,0x47,0x3E,0x91,0xE0,0x40,0x18,0xFE])
    rom[0x150:0x150+len(program)]=program
    check=0
    for byte in rom[0x134:0x14D]: check=(check-byte-1)&0xff
    rom[0x14d]=check
    return bytes(rom)
def main():
    p=ArgumentParser()
    p.add_argument("--out",type=Path,default=Path("STRIPES.GB"))
    a=p.parse_args()
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_bytes(generate())
    print(f"Generated original DMG synthetic ROM: {a.out}")
if __name__=="__main__":main()
