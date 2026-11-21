/* 1stPRTS engine: OGG Vorbis decoding and streaming (stb_vorbis). MIT License. */
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#include "prts/prts_audio.h"

#define DECODE_CHUNK 4096 /* frames per decode call */

static void set_err(char *err, size_t err_size, const char *msg)
{
    if (err && err_size) snprintf(err, err_size, "%s", msg);
}

static stb_vorbis *open_vorbis(const void *data, size_t size, int *channels, int *rate, char *err, size_t err_size)
{
    int error = 0;
    stb_vorbis *v = size <= INT_MAX ? stb_vorbis_open_memory(data, (int)size, &error, NULL) : NULL;
    if (!v) {
        set_err(err, err_size, "not a readable OGG Vorbis file");
        return NULL;
    }
    stb_vorbis_info info = stb_vorbis_get_info(v);
    /* asking stb_vorbis for more channels than the file has pads with silence, so ask for
     * the file's own count (at most stereo; extra channels are dropped) */
    *channels = info.channels >= 2 ? 2 : 1;
    *rate = (int)info.sample_rate;
    return v;
}

prts_sound *prts_sound_decode_ogg(const void *data, size_t size, char *err, size_t err_size)
{
    int ch = 0, rate = 0;
    stb_vorbis *v = open_vorbis(data, size, &ch, &rate, err, err_size);
    if (!v) return NULL;
    size_t cap = DECODE_CHUNK, frames = 0;
    float *pcm = prts_malloc(cap * (size_t)ch * sizeof(float));
    for (;;) {
        if (frames + DECODE_CHUNK > cap) {
            cap *= 2;
            pcm = prts_realloc(pcm, cap * (size_t)ch * sizeof(float));
        }
        int n = stb_vorbis_get_samples_float_interleaved(v, ch, pcm + frames * (size_t)ch, DECODE_CHUNK * ch);
        if (n <= 0 || frames + (size_t)n > (size_t)INT_MAX / 2) break;
        frames += (size_t)n;
    }
    stb_vorbis_close(v);
    if (frames == 0) {
        prts_free(pcm);
        set_err(err, err_size, "OGG Vorbis file has no samples");
        return NULL;
    }
    prts_sound *s = prts_calloc(1, sizeof(*s));
    s->samples = pcm;
    s->frames = (int)frames;
    s->channels = ch;
    s->rate = rate;
    return s;
}

struct prts_stream {
    stb_vorbis *v;
    void *data;
    int channels, rate;
    bool loop, ended;
};

prts_stream *prts_stream_open_ogg(void *data, size_t size, bool loop, char *err, size_t err_size)
{
    int ch = 0, rate = 0;
    stb_vorbis *v = open_vorbis(data, size, &ch, &rate, err, err_size);
    if (!v) {
        prts_free(data);
        return NULL;
    }
    prts_stream *s = prts_calloc(1, sizeof(*s));
    s->v = v;
    s->data = data;
    s->channels = ch;
    s->rate = rate;
    s->loop = loop;
    return s;
}

void prts_stream_destroy(prts_stream *s)
{
    if (!s) return;
    stb_vorbis_close(s->v);
    prts_free(s->data);
    prts_free(s);
}

int prts_stream_channels(const prts_stream *s) { return s->channels; }
int prts_stream_rate(const prts_stream *s) { return s->rate; }

int prts_stream_read(prts_stream *s, float *out, int frames)
{
    int done = 0;
    bool rewound = false;
    while (done < frames && !s->ended) {
        int n = stb_vorbis_get_samples_float_interleaved(s->v, s->channels, out + (size_t)done * (size_t)s->channels,
                                                         (frames - done) * s->channels);
        if (n > 0) {
            done += n;
            rewound = false;
            continue;
        }
        /* end of the data: wrap around, unless the file gives nothing even from the start */
        if (!s->loop || rewound || !stb_vorbis_seek_start(s->v)) s->ended = true;
        rewound = true;
    }
    return done;
}
