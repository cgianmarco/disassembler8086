# Disassembler for 8086 machine code

This project is a simple disassembler for 8086 machine code. It takes a binary file containing 8086 machine code as input and produces an assembly language representation of the code as output.

## Supported Instructions
The disassembler currently supports a subset of 8086 instructions, including:

- **MOV (Register/Memory to/from Register)**: Opcode `100010dw`, handles all addressing modes (register-to-register, memory-to-register, register-to-memory) with 8-bit and 16-bit operands, including direct addressing and displacement modes (8-bit and 16-bit).
- **MOV (Immediate to Register)**: Opcode `1011wreg`, moves immediate 8-bit or 16-bit values directly into registers.
- **MOV (Immediate to Register/Memory)**: Opcode `1100011w`, moves immediate 8-bit or 16-bit values to register or memory locations with explicit size specifiers (`byte` or `word`).
- **MOV (Accumulator to/from Memory)**: Opcode `101000dw`, direct memory access using the accumulator register (AL for 8-bit, AX for 16-bit) with a 16-bit direct address.
- **ADD (Register/Memory with Register to Either)**: Opcode `000000dw`, adds register to register/memory or memory to register with mod-reg-r/m encoding and optional displacement bytes.
- **ADD (Immediate to Register/Memory)**: Opcode `100000sw`, adds immediate data to register/memory locations with mod-000-r/m encoding, optional displacement bytes, and data bytes (sign-extended if s=1 and w=1).

## Usage
To use the disassembler, compile with a C compiler.
```bash
gcc dis8086.c -o dis8086
```
Then run the executable with:
```bash
./dis8086 examples/example1/example1
```