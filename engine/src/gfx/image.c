/* 1stPRTS engine: image decoding and PNG writing (stb). MIT License. */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image.h>
#include <stb_image_write.h>

#include "gfx/image.h"
#include "prts/prts_fs.h"

unsigned char *prts_image_decode(const void *data, size_t size, int *w, int *h)
{
    int comp = 0;
    return stbi_load_from_memory(data, (int)size, w, h, &comp, 4);
}

void prts_image_free(unsigned char *pixels) { stbi_image_free(pixels); }

static void write_cb(void *ctx, void *data, int size) { prts_str_append_n(ctx, data, (size_t)size); }

bool prts_image_write_png(const char *real_path, const unsigned char *rgba, int w, int h)
{
    prts_str buf = {0};
    int ok = stbi_write_png_to_func(write_cb, &buf, w, h, 4, rgba, w * 4);
    bool saved = ok && prts_fs_write_file(real_path, buf.data, buf.len);
    prts_str_free(&buf);
    return saved;
}
