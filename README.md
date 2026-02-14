# Disassembler for 8086 machine code

This project is a simple disassembler for 8086 machine code. It takes a binary file containing 8086 machine code as input and produces an assembly language representation of the code as output.

## Supported Instructions
The disassembler currently supports a subset of 8086 instructions, including:

- **MOV (Register/Memory to/from Register)**: Opcode `100010dw`, handles all addressing modes (register-to-register, memory-to-register, register-to-memory) with 8-bit and 16-bit operands, including direct addressing and displacement modes (8-bit and 16-bit).
- **MOV (Immediate to Register)**: Opcode `1011wreg`, moves immediate 8-bit or 16-bit values directly into registers.

## Usage
To use the disassembler, compile with a C compiler.
```bash
gcc main.c -o dis8086
```
Then run the executable with:
```bash
./dis8086 examples/example1 output.asm
```