/* 1stPRTS engine: decoded sounds, the WAV (RIFF) reader and the placeholder beep. MIT License. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "prts/prts_audio.h"

prts_sound *prts_sound_create(int frames, int channels, int rate)
{
    prts_sound *s = prts_calloc(1, sizeof(*s));
    s->frames = frames;
    s->channels = channels;
    s->rate = rate;
    s->samples = prts_calloc((size_t)frames * (size_t)channels, sizeof(float));
    return s;
}

void prts_sound_destroy(prts_sound *s)
{
    if (!s) return;
    prts_free(s->samples);
    prts_free(s);
}

prts_sound *prts_sound_beep(int rate)
{
    /* 0.15 s at 880 Hz with 5 ms ramps, so the beep itself never clicks */
    int frames = rate * 15 / 100, ramp = rate / 200;
    prts_sound *s = prts_sound_create(frames, 1, rate);
    for (int i = 0; i < frames; i++) {
        float env = 1.0f;
        if (i < ramp) env = (float)i / (float)ramp;
        if (frames - 1 - i < ramp) env = (float)(frames - 1 - i) / (float)ramp;
        double phase = 2.0 * PRTS_PI * 880.0 * (double)i / (double)rate;
        s->samples[i] = 0.35f * env * (float)sin(phase);
    }
    return s;
}

static void set_err(char *err, size_t err_size, const char *msg)
{
    if (err && err_size) snprintf(err, err_size, "%s", msg);
}

static uint32_t rd_u32(const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t rd_u16(const unsigned char *p) { return (uint16_t)(p[0] | p[1] << 8); }

enum { WAV_PCM = 1, WAV_FLOAT = 3, WAV_EXTENSIBLE = 0xFFFE };

typedef struct wav_fmt {
    int format, channels, rate, bits, block;
} wav_fmt;

static bool parse_fmt(const unsigned char *p, uint32_t size, wav_fmt *f, char *err, size_t err_size)
{
    if (size < 16) {
        set_err(err, err_size, "WAV fmt chunk is too short");
        return false;
    }
    f->format = rd_u16(p);
    f->channels = rd_u16(p + 2);
    f->rate = (int)rd_u32(p + 4);
    f->block = rd_u16(p + 12);
    f->bits = rd_u16(p + 14);
    if (f->format == WAV_EXTENSIBLE) {
        if (size < 26) {
            set_err(err, err_size, "WAV extensible fmt chunk is too short");
            return false;
        }
        f->format = rd_u16(p + 24); /* first two bytes of the sub-format GUID */
    }
    if (f->format != WAV_PCM && f->format != WAV_FLOAT) {
        set_err(err, err_size, "WAV encoding is not PCM or float");
        return false;
    }
    if (f->channels < 1 || f->channels > 2) {
        set_err(err, err_size, "WAV must be mono or stereo");
        return false;
    }
    if (f->rate < 1000 || f->rate > 384000) {
        set_err(err, err_size, "WAV sample rate is out of range");
        return false;
    }
    bool ok_bits = f->format == WAV_FLOAT ? (f->bits == 32 || f->bits == 64)
                                          : (f->bits == 8 || f->bits == 16 || f->bits == 24 || f->bits == 32);
    if (!ok_bits || f->block != f->channels * f->bits / 8) {
        set_err(err, err_size, "WAV sample size is not supported");
        return false;
    }
    return true;
}

static float wav_sample(const unsigned char *p, const wav_fmt *f)
{
    if (f->format == WAV_FLOAT) {
        if (f->bits == 32) {
            float v;
            memcpy(&v, p, sizeof(v)); /* little-endian hosts only, like every target platform */
            return v;
        }
        double d;
        memcpy(&d, p, sizeof(d));
        return (float)d;
    }
    switch (f->bits) {
    case 8: return (float)((int)p[0] - 128) / 128.0f;
    case 16: {
        int32_t v = rd_u16(p);
        return (float)(v >= 0x8000 ? v - 0x10000 : v) / 32768.0f;
    }
    case 24: {
        int32_t v = (int32_t)p[0] | (int32_t)p[1] << 8 | (int32_t)p[2] << 16;
        return (float)(v >= 0x800000 ? v - 0x1000000 : v) / 8388608.0f;
    }
    default: {
        int64_t v = (int64_t)rd_u32(p);
        return (float)((double)(v >= 0x80000000LL ? v - 0x100000000LL : v) / 2147483648.0);
    }
    }
}

prts_sound *prts_sound_decode_wav(const void *data, size_t size, char *err, size_t err_size)
{
    const unsigned char *b = data;
    if (size < 12 || memcmp(b, "RIFF", 4) != 0 || memcmp(b + 8, "WAVE", 4) != 0) {
        set_err(err, err_size, "not a RIFF/WAVE file");
        return NULL;
    }
    wav_fmt f = {0, 0, 0, 0, 0};
    bool have_fmt = false;
    size_t at = 12;
    while (at + 8 <= size) {
        const unsigned char *chunk = b + at;
        uint32_t len = rd_u32(chunk + 4);
        size_t avail = size - at - 8;
        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (!parse_fmt(chunk + 8, len <= avail ? len : (uint32_t)avail, &f, err, err_size)) return NULL;
            have_fmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            if (!have_fmt) break;
            /* tolerate a wrong (e.g. streaming placeholder) length: use what is there */
            size_t bytes = len <= avail ? len : avail;
            int frames = (int)PRTS_MIN(bytes / (size_t)f.block, (size_t)0x7fffffff / 2);
            if (frames == 0) {
                set_err(err, err_size, "WAV has no samples");
                return NULL;
            }
            prts_sound *s = prts_sound_create(frames, f.channels, f.rate);
            int bytes_per = f.bits / 8;
            for (int i = 0; i < frames * f.channels; i++) s->samples[i] = wav_sample(chunk + 8 + (size_t)i * (size_t)bytes_per, &f);
            return s;
        }
        if (len > avail) break;
        at += 8 + (size_t)len + (len & 1u);
    }
    set_err(err, err_size, have_fmt ? "WAV has no data chunk" : "WAV has no fmt chunk before its data");
    return NULL;
}

prts_sound *prts_sound_decode(const void *data, size_t size, char *err, size_t err_size)
{
    if (size >= 4 && memcmp(data, "OggS", 4) == 0) return prts_sound_decode_ogg(data, size, err, err_size);
    return prts_sound_decode_wav(data, size, err, err_size);
}
