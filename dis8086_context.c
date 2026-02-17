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
static int fetch8(Context *c, u8 *out)
{
    if (c->cur >= c->size) {
        return 0;
    }
    u8 byte = c->buf[c->cur];
    c->cur += 1;

    // printf("Fetched byte: %s\n", byte_to_string(byte));
    *out = byte;
    return 1;
}

/* Fetch next 16-bit word */
static int fetch16(Context *c, u16 *out)
{
    u8 lo, hi;
    if (!fetch8(c, &lo) || !fetch8(c, &hi)) {
        return 0;
    }

    *out = (u16)(lo | ((u16)hi << 8));
    return 1;
}

/* Load the input file into memory and prepare the context */
static int load_context_from_file(const char *filename, Context *context) {
    FILE *input_file = fopen(filename, "rb");
    if (!input_file) {
        printf("Failed to open input file: %s\n", filename);
        return 0;
    }

    fseek(input_file, 0, SEEK_END);
    long size = ftell(input_file);
    rewind(input_file);

    u8 *buf = malloc(size);
    if (!buf) { 
        fclose(input_file); 
        printf("Failed to allocate memory for input file\n");
        return 0;
    }

    fread(buf, 1, size, input_file);
    fclose(input_file);

    context->buf = buf;
    context->size = size;
    context->cur = 0;

    return 1;
}