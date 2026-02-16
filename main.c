#include "stdio.h"
#include "stdlib.h"

typedef unsigned char u8;
typedef unsigned short u16;


typedef struct {
    u8 * buf;
    size_t size;
    size_t cur;
} Context;

/* Convert a byte to a binary string */
char * byte_to_string(u8 byte) {
    static char buffer[9];
    for(int i = 7; i >= 0; i--) {
        buffer[7 - i] = (byte & (1 << i)) ? '1' : '0';
    }
    buffer[8] = '\0';
    return buffer;
}

/* Fetch next byte */
int fetch8(Context *c, u8 *out)
{
    if (c->cur >= c->size) {
        return 0;
    }
    u8 byte = c->buf[c->cur];
    c->cur += 1;

    printf("Fetched byte: %s\n", byte_to_string(byte));
    *out = byte;
    return 1;
}

/* Fetch next 16-bit word */
int fetch16(Context *c, u16 *out)
{
    u8 lo, hi;
    if (!fetch8(c, &lo) || !fetch8(c, &hi)) {
        return 0;
    }

    *out = (u16)(lo | ((u16)hi << 8));
    return 1;
}

int main(int argc, char *argv[]) {

    if(argc < 3) {
        printf("Usage: dis8086 <input_file> <output_file>\n");
        printf("Example: dis8086 input output.asm\n");
        return 1;
    }


    /* Load the input file into memory */
    /* and prepare the context */
    const char * input_filename = argv[1];

    FILE *input_file = fopen(input_filename, "rb");
    if (!input_file) {
        printf("Failed to open input file: %s\n", input_filename);
        return 1;
    }

    fseek(input_file, 0, SEEK_END);
    long size = ftell(input_file);
    rewind(input_file);

    u8 *buf = malloc(size);
    if (!buf) { 
        fclose(input_file); 
        printf("Failed to allocate memory for input file\n");
        return 1;
    }

    fread(buf, 1, size, input_file);
    fclose(input_file);

    Context context = {
        .buf = buf,
        .size = size,
        .cur = 0
    };



    const char * output_filename = argv[2];
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

    u8 instr;

    while (fetch8(&context, &instr)) {
        char decoded[32];

        // MOV
        if ((instr & 0b11111100) == 0b10001000) {
            
            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            u8 modrm;
            if (!fetch8(&context, &modrm)) {
                printf("Failed to fetch ModR/M byte\n");
                return 1;
            }

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
                        short d16;
                        if (!fetch16(&context, (u16 *)&d16)) {
                            printf("Failed to fetch 16-bit displacement\n");
                            return 1;
                        }
                        snprintf(rm_buf, sizeof(rm_buf), "[%d]", d16);
                        decoded_rm = rm_buf;
                    } else {
                        snprintf(rm_buf, sizeof(rm_buf), "[%s]", Patterns[rm]);
                        decoded_rm = rm_buf;
                    }
                    break;
                case 0b01:                    
                    char disp8;
                    if (!fetch8(&context, (u8 *)&disp8)) {
                        printf("Failed to fetch 8-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp8);
                    decoded_rm = rm_buf;
                    break;
                case 0b10:                    
                    short disp16;
                    if (!fetch16(&context, (u16 *)&disp16)) {
                        printf("Failed to fetch 16-bit displacement\n");
                        return 1;
                    }
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
                char imm;
                if (!fetch8(&context, (u8 *)&imm)) {
                    printf("Failed to fetch 8-bit immediate\n");
                    return 1;
                }
                snprintf(imm_buf, sizeof(imm_buf), "%d", imm);
            } else {
                short imm;
                if (!fetch16(&context, (u16 *)&imm)) {
                    printf("Failed to fetch 16-bit immediate\n");
                    return 1;
                }
                snprintf(imm_buf, sizeof(imm_buf), "%d", imm);
            }

            printf("Decoded instruction: mov %s, %s\n", decoded_reg, imm_buf);
            fprintf(output_file, "mov %s, %s\n", decoded_reg, imm_buf);

        } else if ((instr & 0b11111110) == 0b11000110) {

            u8 w = instr & 0b1;

            u8 modrm;
            if (!fetch8(&context, &modrm)) {
                printf("Failed to fetch ModR/M byte\n");
                return 1;
            }

            // reg field expected to be 000
            u8 mod = (modrm >> 6) & 0b11;
            u8 rm = modrm & 0b111;

            const char *decoded_rm = NULL;

            char rm_buf[32];
            char imm_buf[32];

            switch (mod) {
                case 0b11:
                    decoded_rm = Registers[w][rm];
                    break;
                case 0b00:
                    if (rm == 0b110) {
                        short d16;
                        if (!fetch16(&context, (u16 *)&d16)) {
                            printf("Failed to fetch 16-bit displacement\n");
                            return 1;
                        }
                        snprintf(rm_buf, sizeof(rm_buf), "[%d]", d16);
                        decoded_rm = rm_buf;
                    } else {
                        snprintf(rm_buf, sizeof(rm_buf), "[%s]", Patterns[rm]);
                        decoded_rm = rm_buf;
                    }
                    break;
                case 0b01:                    
                    char disp8;
                    if (!fetch8(&context, (u8 *)&disp8)) {
                        printf("Failed to fetch 8-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp8);
                    decoded_rm = rm_buf;
                    break;
                case 0b10:                    
                    short disp16;
                    if (!fetch16(&context, (u16 *)&disp16)) {
                        printf("Failed to fetch 16-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp16);
                    decoded_rm = rm_buf;
                    break;
            }

            if(w == 0){
                char imm;
                if (!fetch8(&context, (u8 *)&imm)) {
                    printf("Failed to fetch 8-bit immediate\n");
                    return 1;
                }
                snprintf(imm_buf, sizeof(imm_buf), "byte %d", imm);
            } else {
                short imm;
                if (!fetch16(&context, (u16 *)&imm)) {
                    printf("Failed to fetch 16-bit immediate\n");
                    return 1;
                }
                snprintf(imm_buf, sizeof(imm_buf), "word %d", imm);
            }

            const char *dst = decoded_rm;
            const char *src = imm_buf;

            printf("Decoded instruction: mov %s, %s\n", dst, src);
            fprintf(output_file, "mov %s, %s\n", dst, src);

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

            printf("Decoded instruction: mov %s, %s\n", dst, src);
            fprintf(output_file, "mov %s, %s\n", dst, src);

        // ADD
        } else if((instr & 0b11111100) == 0b0) {

            u8 d = (instr >> 1) & 1;
            u8 w = instr & 1;

            u8 modrm;
            if (!fetch8(&context, &modrm)) {
                printf("Failed to fetch ModR/M byte\n");
                return 1;
            }

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
                        short d16;
                        if (!fetch16(&context, (u16 *)&d16)) {
                            printf("Failed to fetch 16-bit displacement\n");
                            return 1;
                        }
                        snprintf(rm_buf, sizeof(rm_buf), "[%d]", d16);
                        decoded_rm = rm_buf;
                    } else {
                        snprintf(rm_buf, sizeof(rm_buf), "[%s]", Patterns[rm]);
                        decoded_rm = rm_buf;
                    }
                    break;
                case 0b01:                    
                    char disp8;
                    if (!fetch8(&context, (u8 *)&disp8)) {
                        printf("Failed to fetch 8-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp8);
                    decoded_rm = rm_buf;
                    break;
                case 0b10:                    
                    short disp16;
                    if (!fetch16(&context, (u16 *)&disp16)) {
                        printf("Failed to fetch 16-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "[%s + %d]", Patterns[rm], disp16);
                    decoded_rm = rm_buf;
                    break;
            }

            const char *dst = d ? decoded_reg : decoded_rm;
            const char *src = d ? decoded_rm : decoded_reg;

            printf("Decoded instruction: add %s, %s\n", dst, src);
            fprintf(output_file, "add %s, %s\n", dst, src);

        } else if ((instr & 0b11111100) == 0b10000000) {

            u8 s = (instr >> 1) & 1;
            u8 w = instr & 1;

            u8 modrm;
            if (!fetch8(&context, &modrm)) {
                printf("Failed to fetch ModR/M byte\n");
                return 1;
            }

            u8 mod = (modrm >> 6) & 0b11;
            u8 reg = (modrm >> 3) & 0b111; // reg field expected to be 000 for ADD
            u8 rm = modrm & 0b111;

            const char *decoded_rm = NULL;

            char rm_buf[32];

            char * size_str = w ? "word" : "byte";

            switch (mod) {
                case 0b11:
                    decoded_rm = Registers[w][rm];
                    break;
                case 0b00:
                    if (rm == 0b110) {
                        short d16;
                        if (!fetch16(&context, (u16 *)&d16)) {
                            printf("Failed to fetch 16-bit displacement\n");
                            return 1;
                        }
                        snprintf(rm_buf, sizeof(rm_buf), "%s [%d]", size_str, d16);
                        decoded_rm = rm_buf;
                    } else {
                        snprintf(rm_buf, sizeof(rm_buf), "%s [%s]", size_str, Patterns[rm]);
                        decoded_rm = rm_buf;
                    }
                    break;
                case 0b01:                    
                    char disp8;
                    if (!fetch8(&context, (u8 *)&disp8)) {
                        printf("Failed to fetch 8-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "%s [%s + %d]", size_str, Patterns[rm], disp8);
                    decoded_rm = rm_buf;
                    break;
                case 0b10:                    
                    short disp16;
                    if (!fetch16(&context, (u16 *)&disp16)) {
                        printf("Failed to fetch 16-bit displacement\n");
                        return 1;
                    }
                    snprintf(rm_buf, sizeof(rm_buf), "%s [%s + %d]", size_str, Patterns[rm], disp16);
                    decoded_rm = rm_buf;
                    break;
            }

            char imm_buf[32];

            if (w == 0) {
                char imm;
                if (!fetch8(&context, (u8 *)&imm)) {
                    printf("Failed to fetch 8-bit immediate\n");
                    return 1;
                }
                snprintf(imm_buf, sizeof(imm_buf), "%d", imm);
            } else {
                if (s == 0) {
                    short imm;
                    if (!fetch16(&context, (u16 *)&imm)) {
                        printf("Failed to fetch 16-bit immediate\n");
                        return 1;
                    }
                    snprintf(imm_buf, sizeof(imm_buf), "%d", imm);
                    
                } else {
                    char imm8_signed;
                    if (!fetch8(&context, (u8 *)&imm8_signed)) {
                        printf("Failed to fetch 8-bit immediate\n");
                        return 1;
                    }
                    short imm8 = (short)imm8_signed;
                    snprintf(imm_buf, sizeof(imm_buf), "%d", imm8);
                }
            }

            const char *dst = decoded_rm;
            const char *src = imm_buf;

            printf("Decoded instruction: add %s, %s\n", dst, src);
            fprintf(output_file, "add %s, %s\n", dst, src);

        } else {

            printf("Unknown instruction: %s\n", byte_to_string(instr));
            return 1;

        }
    }

    free(buf);
}