# Disassembler for 8086 machine code

This project is a simple disassembler for 8086 machine code. It takes a binary file containing 8086 machine code as input and produces an assembly language representation of the code as output.

## Supported Instructions
The disassembler currently supports a subset of 8086 instructions:

- **MOV (Register/Memory to/from Register)**: `100010dw`
- **MOV (Immediate to Register)**: `1011wreg`
- **MOV (Immediate to Register/Memory)**: `1100011w`
- **MOV (Accumulator to/from Memory)**: `101000dw`
- **ADD (Register/Memory with Register)**: `000000dw`
- **ADD (Immediate to Register/Memory)**: `100000sw`
- **ADD (Immediate to Accumulator)**: `0000010w`
- **SUB (Register/Memory with Register)**: `001010dw`
- **SUB (Immediate from Register/Memory)**: `100000sw`
- **SUB (Immediate from Accumulator)**: `0010110w`
- **CMP (Register/Memory with Register)**: `001110dw`
- **CMP (Immediate with Register/Memory)**: `100000sw`
- **CMP (Immediate with Accumulator)**: `0011110w`

## Usage
To use the disassembler, compile with a C compiler.
```bash
gcc dis8086.c -o dis8086
```
Then run the executable with:
```bash
./dis8086 examples/example1/example1
```