typedef struct {
    u8 mod, reg, rm;
    short disp;
    u8 disp_size;
} ModRM;

static int decode_modrm(Context *context, ModRM *modrm);
static int decode_imm(Context *context, short *imm, const u8 w, const u8 s);