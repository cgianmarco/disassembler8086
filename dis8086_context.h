typedef struct {
    u8 * buf;
    size_t size;
    size_t cur;
} Context;

static int fetch8(Context *c, u8 *out);
static int fetch16(Context *c, u16 *out);
static int load_context_from_file(const char *filename, Context *context);