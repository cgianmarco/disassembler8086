static int decode_modrm(Context *context, ModRM *modrm) {
    u8 byte;
    if (!fetch8(context, &byte)) {
        printf("Failed to fetch ModR/M byte\n");
        return 0;
    }

    modrm->mod = (byte >> 6) & 0b11;
    modrm->reg = (byte >> 3) & 0b111;
    modrm->rm = byte & 0b111;

    modrm->disp_size = 0;
    modrm->disp = 0;

    switch (modrm->mod) {
        case 0b00:
            if (modrm->rm == 0b110) {
                short d16;
                if (!fetch16(context, (u16 *)&d16)) {
                    printf("Failed to fetch 16-bit displacement\n");
                    return 0;
                }
                modrm->disp = d16;
                modrm->disp_size = 2;
            }
            break;
        case 0b01:                    
            char disp8;
            if (!fetch8(context, (u8 *)&disp8)) {
                printf("Failed to fetch 8-bit displacement\n");
                return 0;
            }
            modrm->disp = (u16)(char)disp8;
            modrm->disp_size = 1;
            break;
        case 0b10:                    
            short disp16;
            if (!fetch16(context, (u16 *)&disp16)) {
                printf("Failed to fetch 16-bit displacement\n");
                return 0;
            }
            modrm->disp = disp16;
            modrm->disp_size = 2;
            break;
    }

    return 1;
}

static int decode_imm(Context *context, short *imm, const u8 w, const u8 s) {
    if(w == 0){
        char imm8;
        if (!fetch8(context, (u8 *)&imm8)) {
            printf("Failed to fetch 8-bit immediate\n");
            return 1;
        }
        *imm = (short) imm8; 
    } else {
        if (s == 0) {
             short imm16;
             if (!fetch16(context, (u16 *)&imm16)) {
                printf("Failed to fetch 16-bit immediate\n");
                return 1;
            }
            *imm = imm16;
        } else {
            char imm8_signed;
            if (!fetch8(context, (u8 *)&imm8_signed)) {
                printf("Failed to fetch 8-bit immediate\n");
                return 1;
            }
            *imm = (short) imm8_signed; 
        }
    }
}