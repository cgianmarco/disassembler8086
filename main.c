#include "stdio.h"

typedef unsigned char u8;
typedef unsigned short u16;

/* Convert a byte to a binary string */
char * byte_to_string(u8 byte) {
    static char buffer[9];
    for(int i = 7; i >= 0; i--) {
        buffer[7 - i] = (byte & (1 << i)) ? '1' : '0';
    }
    buffer[8] = '\0';
    return buffer;
}

/* Fetch a byte from file */
u8 fetch8(FILE *f)
{
    u8 byte = 0;
    if (fread(&byte, 1, 1, f) != 1) return 0;
    printf("Fetched byte: %s\n", byte_to_string(byte));
    return byte;
}

/* Fetch a 16-bit word from file */
u16 fetch16(FILE *f)
{
    u8 lo = 0;
    u8 hi = 0;

    if (fread(&lo, 1, 1, f) != 1) return 0;
    if (fread(&hi, 1, 1, f) != 1) return 0;

    printf("Fetched word: %s %s\n", byte_to_string(hi), byte_to_string(lo));

    return (u16)(lo | ((u16)hi << 8));
}

int main(int argc, char *argv[]) {

    if(argc < 3) {
        printf("Usage: dis8086 <input_file> <output_file>\n");
        printf("Example: dis8086 input output.asm\n");
        return 1;
    }

    const char * input_filename = argv[1];
    const char * output_filename = argv[2];

    FILE *input_file = fopen(input_filename, "rb");
    if (!input_file) {
        printf("Failed to open input file: %s\n", input_filename);
        return 1;
    }

    FILE *output_file = fopen(output_filename, "w+");
    if (!output_file) {
        printf("Failed to open output file: %s\n", output_filename);
        return 1;
    }

    const char * Registers[2][8] = {
        {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
        {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
    };

    const char * Patterns[] = {
        "bx + si",
        "bx + di",
        "bp + si",
        "bp + di",
        "si",
        "di",
        "bp",
        "bx"
    };

    u8 instr = fetch8(input_file);

    while (instr != 0) {
        char decoded[32];

        if ((instr & 0b11111100) == 0b10001000) {
            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            u8 modrm = fetch8(input_file);

            u8 mod = (modrm >> 6) & 0b11;
            u8 reg = (modrm >> 3) & 0b111;
            u8 rm = modrm & 0b111;

            const char *decoded_reg = Registers[w][reg];
            const char *decoded_rm = NULL;

            char rm_buf[32];

            switch (mod) {
                case 0b11:
                    decoded_rm = Registers[w][rm];
                    break;
                case 0b00:
                    if (rm == 0b110) {
                        short d16 = (short) fetch16(input_file);
                        snprintf(rm_buf, sizeof(rm_buf), "[%d]", d16);
                        decoded_rm = rm_buf;
                    } else {
                        snprintf(rm_buf, sizeof(rm_buf), "[%s]", Patterns[rm]);
                        decoded_rm = rm_buf;
                    }
                    break;
                case 0b01:                    
                    char disp8 = (char) fetch8(input_file);
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp8);
                    decoded_rm = rm_buf;
                    break;
                case 0b10:                    
                    short disp16 = (short) fetch16(input_file);
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp16);
                    decoded_rm = rm_buf;
                    break;
            }

            const char *dst = d ? decoded_reg : decoded_rm;
            const char *src = d ? decoded_rm : decoded_reg;

            printf("Decoded instruction: mov %s, %s\n", dst, src);
            fprintf(output_file, "mov %s, %s\n", dst, src);

        } else if ((instr & 0b11110000) == 0b10110000) {

            unsigned char w = (instr >> 3) & 0b1;
            unsigned char reg = instr & 0b111;

            const char *decoded_reg = Registers[w][reg];
            char imm_buf[32];

            if(w == 0){
                char imm = (char) fetch8(input_file);
                snprintf(imm_buf, sizeof(imm_buf), "%d", imm);
            } else {
                short imm = (short) fetch16(input_file);
                snprintf(imm_buf, sizeof(imm_buf), "%d", imm);
            }

            printf("Decoded instruction: mov %s, %s\n", decoded_reg, imm_buf);
            fprintf(output_file, "mov %s, %s\n", decoded_reg, imm_buf);
        }

        instr = fetch8(input_file);
    }
}