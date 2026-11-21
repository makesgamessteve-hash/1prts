/* 1stPRTS engine: image decoding and PNG writing. MIT License. */
#ifndef PRTS_IMAGE_H
#define PRTS_IMAGE_H

#include "prts/prts_base.h"

/* Decodes a PNG into RGBA8. Returns NULL on failure. */
unsigned char *prts_image_decode(const void *data, size_t size, int *w, int *h);
void prts_image_free(unsigned char *pixels);
bool prts_image_write_png(const char *real_path, const unsigned char *rgba, int w, int h);

#endif
