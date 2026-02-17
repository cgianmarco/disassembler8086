#include "stdio.h"
#include "stdlib.h"

#include "dis8086.h"
#include "dis8086_context.h"
#include "dis8086_context.c"

#include "dis8086_decode.h"
#include "dis8086_decode.c"

static const char * Registers[2][8] = {
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
};

static const char * Patterns[] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx"
};


void format_rm(char *out, size_t n, ModRM *modrm, u8 w, u8 use_size_prefix) {

    char size_prefix[16] = "";
    if (use_size_prefix) {
        snprintf(size_prefix, sizeof(size_prefix), "%s ", w ? "word" : "byte");
    }
    
    switch (modrm->mod) {
        case 0b11:
            snprintf(out, n, "%s", Registers[w][modrm->rm]);
            break;
        case 0b00:
            if (modrm->rm == 0b110) {
                snprintf(out, n, "%s[%d]", size_prefix, modrm->disp);
            } else {
                snprintf(out, n, "%s[%s]", size_prefix, Patterns[modrm->rm]);
            }
            break;
        case 0b01:
            snprintf(out, n, "%s[%s + %d]", size_prefix, Patterns[modrm->rm], (char)modrm->disp);
            break;
        case 0b10:
            snprintf(out, n, "%s[%s + %d]", size_prefix, Patterns[modrm->rm], modrm->disp);
            break;
    }
}


int main(int argc, char *argv[]) {

    if(argc < 2) {
        printf("Usage: dis8086 <input_file>\n");
        printf("Example: dis8086 input\n");
        return 1;
    }

    const char * input_filename = argv[1];

    Context context;
    if (!load_context_from_file(input_filename, &context)) {
        printf("Failed to load input file into context\n");
        return 1;
    }

    printf("bits 16\n");

    u8 instr;

    while (fetch8(&context, &instr)) {
        char decoded[32];

        // MOV
        if ((instr & 0b11111100) == 0b10001000) {
            
            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            ModRM modrm;
            if(!decode_modrm(&context, &modrm)) {
                printf("Failed to decode ModR/M byte\n");
                return 1;
            }

            const char *decoded_reg = Registers[w][modrm.reg];

            char decoded_rm[32];
            format_rm(decoded_rm, sizeof(decoded_rm), &modrm, w, 0);

            const char *dst = d ? decoded_reg : decoded_rm;
            const char *src = d ? decoded_rm : decoded_reg;

            snprintf(decoded, sizeof(decoded), "mov %s, %s", dst, src);

        } else if ((instr & 0b11110000) == 0b10110000) {

            unsigned char w = (instr >> 3) & 0b1;
            unsigned char reg = instr & 0b111;

            const char *decoded_reg = Registers[w][reg];
            
            short imm;
            if(!decode_imm(&context, &imm, w, 0)) {
                printf("Failed to decode immediate value\n");
                return 1;
            }

            sprintf(decoded, "mov %s, %d", decoded_reg, imm);

        } else if ((instr & 0b11111110) == 0b11000110) {

            u8 w = instr & 0b1;

            ModRM modrm;
            if(!decode_modrm(&context, &modrm)) {
                printf("Failed to decode ModR/M byte\n");
                return 1;
            }

            char decoded_rm[32];
            format_rm(decoded_rm, sizeof(decoded_rm), &modrm, w, 1);
            

            short imm;
            if(!decode_imm(&context, &imm, w, 0)) {
                printf("Failed to decode immediate value\n");
                return 1;
            }

            snprintf(decoded, sizeof(decoded), "mov %s, %d", decoded_rm, imm);

        } else if ((instr & 0b11111100) == 0b10100000) {

            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            const char *decoded_reg = Registers[w][0];

            char rm_buf[32];

            short d16;
            if (!fetch16(&context, (u16 *)&d16)) {
                printf("Failed to fetch 16-bit displacement\n");
                return 1;
            }
            snprintf(rm_buf, sizeof(rm_buf), "[%d]", d16);
            const char *decoded_rm = rm_buf;

            const char *dst = d ? decoded_rm : decoded_reg;
            const char *src = d ? decoded_reg : decoded_rm;

            snprintf(decoded, sizeof(decoded), "mov %s, %s", dst, src);

        // ADD
        } else if((instr & 0b11111100) == 0b0) {

            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            ModRM modrm;
            if(!decode_modrm(&context, &modrm)) {
                printf("Failed to decode ModR/M byte\n");
                return 1;
            }

            const char *decoded_reg = Registers[w][modrm.reg];
            char decoded_rm[32];
            format_rm(decoded_rm, sizeof(decoded_rm), &modrm, w, 0);

            const char *dst = d ? decoded_reg : decoded_rm;
            const char *src = d ? decoded_rm : decoded_reg;

            snprintf(decoded, sizeof(decoded), "add %s, %s", dst, src);

        } else if ((instr & 0b11111100) == 0b10000000) {

            u8 s = (instr >> 1) & 1;
            u8 w = instr & 1;

            ModRM modrm;
            if(!decode_modrm(&context, &modrm)) {
                printf("Failed to decode ModR/M byte\n");
                return 1;
            }

            char decoded_rm[32];
            format_rm(decoded_rm, sizeof(decoded_rm), &modrm, w, 1);

            short imm;
            if(!decode_imm(&context, &imm, w, s)) {
                printf("Failed to decode immediate value\n");
                return 1;
            }

            switch (modrm.reg) {
                case 0b000: 
                    snprintf(decoded, sizeof(decoded), "add %s, %d", decoded_rm, imm);
                    break;
                case 0b101: 
                    snprintf(decoded, sizeof(decoded), "sub %s, %d", decoded_rm, imm);
                    break;
                default:
                    snprintf(decoded, sizeof(decoded), "Unknown ADD opcode with reg field: %d", modrm.reg);
                    break;
            }

        } else if ((instr & 0b11111110) == 0b00000100) { 

            u8 w = instr & 1;

            const char *decoded_reg = Registers[w][0];

            short imm;
            if(!decode_imm(&context, &imm, w, 0)) {
                printf("Failed to decode immediate value\n");
                return 1;
            }

            snprintf(decoded, sizeof(decoded), "add %s, %d", decoded_reg, imm);

        // SUB
        } else if((instr & 0b11111100) == 0b00101000) {

            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            ModRM modrm;
            if(!decode_modrm(&context, &modrm)) {
                printf("Failed to decode ModR/M byte\n");
                return 1;
            }

            const char *decoded_reg = Registers[w][modrm.reg];
            char decoded_rm[32];
            format_rm(decoded_rm, sizeof(decoded_rm), &modrm, w, 0);

            const char *dst = d ? decoded_reg : decoded_rm;
            const char *src = d ? decoded_rm : decoded_reg;

            snprintf(decoded, sizeof(decoded), "sub %s, %s", dst, src);

        } else if ((instr & 0b11111110) == 0b00101100) { 

            u8 w = instr & 1;

            const char *decoded_reg = Registers[w][0];

            short imm;
            if(!decode_imm(&context, &imm, w, 0)) {
                printf("Failed to decode immediate value\n");
                return 1;
            }

            snprintf(decoded, sizeof(decoded), "sub %s, %d", decoded_reg, imm);

        } else {

            printf("Unknown instruction: %s\n", byte_to_string(instr));
            return 1;

        }

        printf("%s\n", decoded);
    }
}