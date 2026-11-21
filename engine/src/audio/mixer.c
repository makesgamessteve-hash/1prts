/* 1stPRTS engine: the float mixer (voices, buses, attenuation, panning, muffling). MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_audio.h"

#define SUB_BLOCK 256    /* frames per gain ramp */
#define STREAM_BUF 2048  /* frames of a stream decoded ahead */
#define MIN_FADE 0.005f  /* seconds: the shortest stop, so stopping never clicks */
#define DEFAULT_REF 1.5f
#define DEFAULT_MAX 40.0f
#define MUFFLE_GAIN 0.6f /* gain lost when fully muffled */
#define MUFFLE_TOP_HZ 20000.0f
#define MUFFLE_BOTTOM_HZ 350.0f

typedef struct voice {
    bool active;
    bool started; /* gains have been initialised by a first block */
    uint32_t gen;
    const prts_sound *sound;
    prts_stream *stream;
    float *sbuf;         /* stream window: decoded frames from sbuf_start */
    int64_t sbuf_start;
    int sbuf_len;
    bool stream_ended;
    int channels;
    double pos, step;    /* in source frames */
    prts_audio_bus bus;
    float volume;
    bool loop, positional;
    prts_dvec3 at;
    float ref, max;
    float fade, fade_target, fade_rate; /* fade_rate: change per output frame */
    bool stop_at_end;
    float muffle;    /* set by scripts */
    float occlusion; /* set by the engine from interior cells and portals */
    float lp[2];         /* low-pass state per channel */
    float gl, gr;        /* total gains at the end of the last block */
    uint64_t tag;
} voice;

struct prts_mixer {
    int rate;
    float bus[PRTS_BUS_COUNT];
    prts_dvec3 listener;
    float yaw;
    voice v[PRTS_AUDIO_MAX_VOICES];
};

static const char *BUS_NAMES[PRTS_BUS_COUNT] = {"master", "effects", "voice", "music", "ambience"};

const char *prts_audio_bus_name(prts_audio_bus bus)
{
    return (unsigned)bus < (unsigned)PRTS_BUS_COUNT ? BUS_NAMES[bus] : "?";
}

bool prts_audio_bus_parse(const char *name, prts_audio_bus *out)
{
    for (int i = 0; i < PRTS_BUS_COUNT; i++) {
        if (name && strcmp(name, BUS_NAMES[i]) == 0) {
            *out = (prts_audio_bus)i;
            return true;
        }
    }
    return false;
}

/* clamps, mapping NaN to lo */
static float clampf(float v, float lo, float hi) { return fminf(fmaxf(v, lo), hi); }

void prts_audio_pan_gains(float pan, float *left, float *right)
{
    float angle = (clampf(pan, -1.0f, 1.0f) + 1.0f) * (float)(PRTS_PI / 4.0);
    *left = cosf(angle);
    *right = sinf(angle);
}

float prts_audio_attenuation(float distance, float ref_distance, float max_distance)
{
    float ref = ref_distance > 0.0f ? ref_distance : DEFAULT_REF;
    float max = max_distance > ref ? max_distance : ref + DEFAULT_MAX;
    if (!(distance >= 0.0f) || distance >= max) return 0.0f;
    if (distance <= ref) return 1.0f;
    /* inverse distance, tapered to reach silence at max */
    return ref / distance * (max - distance) / (max - ref);
}

float prts_audio_muffle_cutoff(float amount)
{
    float m = clampf(amount, 0.0f, 1.0f);
    if (m <= 0.0f) return 0.0f;
    return MUFFLE_TOP_HZ * powf(MUFFLE_BOTTOM_HZ / MUFFLE_TOP_HZ, m);
}

void prts_voice_params_default(prts_voice_params *p)
{
    memset(p, 0, sizeof(*p));
    p->bus = PRTS_BUS_EFFECTS;
    p->volume = 1.0f;
    p->pitch = 1.0f;
}

prts_mixer *prts_mixer_create(int rate)
{
    prts_mixer *m = prts_calloc(1, sizeof(*m));
    m->rate = rate > 0 ? rate : PRTS_AUDIO_RATE;
    for (int i = 0; i < PRTS_BUS_COUNT; i++) m->bus[i] = 1.0f;
    return m;
}

static void free_voice(voice *v)
{
    prts_stream_destroy(v->stream);
    prts_free(v->sbuf);
    v->stream = NULL;
    v->sbuf = NULL;
    v->sound = NULL;
    v->active = false;
}

void prts_mixer_stop_all(prts_mixer *m)
{
    for (int i = 0; i < PRTS_AUDIO_MAX_VOICES; i++) {
        if (m->v[i].active) free_voice(&m->v[i]);
    }
}

void prts_mixer_destroy(prts_mixer *m)
{
    if (!m) return;
    prts_mixer_stop_all(m);
    prts_free(m);
}

int prts_mixer_rate(const prts_mixer *m) { return m->rate; }

static voice *find(const prts_mixer *m, prts_voice_id id)
{
    int slot = (int)(id & 0xffu) - 1;
    if (slot < 0 || slot >= PRTS_AUDIO_MAX_VOICES) return NULL;
    const voice *v = &m->v[slot];
    return v->active && v->gen == id >> 8 ? (voice *)v : NULL;
}

prts_voice_id prts_mixer_play(prts_mixer *m, const prts_voice_params *p)
{
    const prts_sound *s = p->sound;
    bool ok = s ? s->frames > 0 && (s->channels == 1 || s->channels == 2) && s->rate > 0 && s->samples != NULL
                : p->stream != NULL;
    int slot = -1;
    for (int i = 0; ok && i < PRTS_AUDIO_MAX_VOICES && slot < 0; i++) {
        if (!m->v[i].active) slot = i;
    }
    if (p->stream && (slot < 0 || s)) prts_stream_destroy(p->stream); /* not playable, or not used */
    if (slot < 0) return PRTS_VOICE_NONE;
    voice *v = &m->v[slot];
    uint32_t gen = (v->gen + 1u) & 0xffffffu;
    memset(v, 0, sizeof(*v));
    v->gen = gen ? gen : 1u;
    v->active = true;
    int src_rate;
    if (s) {
        v->sound = s;
        v->channels = s->channels;
        src_rate = s->rate;
    } else {
        v->stream = p->stream;
        v->channels = prts_stream_channels(p->stream);
        src_rate = prts_stream_rate(p->stream);
        v->sbuf = prts_malloc((size_t)STREAM_BUF * (size_t)v->channels * sizeof(float));
    }
    v->step = (double)src_rate / (double)m->rate * (double)clampf(p->pitch > 0.0f ? p->pitch : 1.0f, 0.5f, 2.0f);
    v->loop = p->loop;
    v->bus = (unsigned)p->bus < (unsigned)PRTS_BUS_COUNT ? p->bus : PRTS_BUS_EFFECTS;
    v->volume = clampf(p->volume, 0.0f, 8.0f);
    v->positional = p->positional;
    v->at = p->pos;
    v->ref = p->ref_distance;
    v->max = p->max_distance;
    v->fade_target = 1.0f;
    if (p->fade_in > 0.0f) v->fade_rate = 1.0f / (p->fade_in * (float)m->rate);
    else v->fade = 1.0f;
    v->tag = p->tag;
    return (v->gen << 8) | (uint32_t)(slot + 1);
}

bool prts_mixer_playing(const prts_mixer *m, prts_voice_id id) { return find(m, id) != NULL; }

bool prts_mixer_fade(prts_mixer *m, prts_voice_id id, float target, float seconds, bool stop_at_end)
{
    voice *v = find(m, id);
    if (!v) return false;
    v->fade_target = clampf(target, 0.0f, 1.0f);
    float secs = fmaxf(seconds, MIN_FADE);
    v->fade_rate = fmaxf(fabsf(v->fade_target - v->fade), 1e-6f) / (secs * (float)m->rate);
    v->stop_at_end = stop_at_end;
    return true;
}

bool prts_mixer_stop(prts_mixer *m, prts_voice_id id, float fade_seconds)
{
    voice *v = find(m, id);
    if (!v) return false;
    /* a full-scale fade takes fade_seconds; stopping again never slows a stop down */
    float rate = 1.0f / (fmaxf(fade_seconds, MIN_FADE) * (float)m->rate);
    if (v->stop_at_end && v->fade_target == 0.0f) rate = fmaxf(rate, v->fade_rate);
    v->fade_target = 0.0f;
    v->fade_rate = rate;
    v->stop_at_end = true;
    return true;
}

bool prts_mixer_set_volume(prts_mixer *m, prts_voice_id id, float volume)
{
    voice *v = find(m, id);
    if (v) v->volume = clampf(volume, 0.0f, 8.0f);
    return v != NULL;
}

bool prts_mixer_set_position(prts_mixer *m, prts_voice_id id, prts_dvec3 pos)
{
    voice *v = find(m, id);
    if (v) v->at = pos;
    return v != NULL;
}

bool prts_mixer_set_muffle(prts_mixer *m, prts_voice_id id, float amount)
{
    voice *v = find(m, id);
    if (v) v->muffle = clampf(amount, 0.0f, 1.0f);
    return v != NULL;
}

bool prts_mixer_set_occlusion(prts_mixer *m, prts_voice_id id, float amount)
{
    voice *v = find(m, id);
    if (v) v->occlusion = clampf(amount, 0.0f, 1.0f);
    return v != NULL;
}

/* What the listener hears: the stronger of the script's muffle and the walls' occlusion. */
static float heard_muffle(const voice *v) { return v->muffle > v->occlusion ? v->muffle : v->occlusion; }

void prts_mixer_set_bus_volume(prts_mixer *m, prts_audio_bus bus, float volume)
{
    if ((unsigned)bus < (unsigned)PRTS_BUS_COUNT) m->bus[bus] = clampf(volume, 0.0f, 1.0f);
}

float prts_mixer_bus_volume(const prts_mixer *m, prts_audio_bus bus)
{
    return (unsigned)bus < (unsigned)PRTS_BUS_COUNT ? m->bus[bus] : 0.0f;
}

void prts_mixer_set_listener(prts_mixer *m, prts_dvec3 pos, float yaw)
{
    m->listener = pos;
    m->yaw = yaw;
}

/* Fades follow an equal-power curve, so a cross-fade between two tracks keeps its loudness. */
static float fade_gain(float fade)
{
    return fade >= 1.0f ? 1.0f : fade <= 0.0f ? 0.0f : sinf(fade * (float)(PRTS_PI / 2.0));
}

/* Total left/right gain of a voice for a given fade value. */
static void voice_gains(const prts_mixer *m, const voice *v, float fade, float *gl, float *gr)
{
    float g = v->volume * fade_gain(fade) * m->bus[PRTS_BUS_MASTER] * (v->bus != PRTS_BUS_MASTER ? m->bus[v->bus] : 1.0f);
    g *= 1.0f - MUFFLE_GAIN * heard_muffle(v);
    if (!v->positional) {
        *gl = *gr = g;
        return;
    }
    prts_vec3 rel = prts_dv3_relative(v->at, m->listener);
    float d = prts_v3_length(rel);
    g *= prts_audio_attenuation(d, v->ref, v->max);
    /* the listener's right is +x at yaw 0 (looking along -z) */
    prts_vec3 right = prts_v3(cosf(m->yaw), 0.0f, -sinf(m->yaw));
    float pan = d > 1e-4f ? prts_v3_dot(rel, right) / d : 0.0f;
    float l = 0.0f, r = 0.0f;
    prts_audio_pan_gains(pan, &l, &r);
    *gl = g * l;
    *gr = g * r;
}

static float approach(float x, float target, float delta)
{
    return x < target ? fminf(x + delta, target) : fmaxf(x - delta, target);
}

/* Makes sure stream frames idx and idx + 1 are in the window, when the stream has them. */
static void stream_need(voice *v, int64_t idx)
{
    int ch = v->channels;
    while (idx + 1 >= v->sbuf_start + v->sbuf_len && !v->stream_ended) {
        int64_t end = v->sbuf_start + v->sbuf_len;
        int64_t from = idx < end ? idx : end;
        int keep = (int)(end - from);
        memmove(v->sbuf, v->sbuf + (size_t)(from - v->sbuf_start) * (size_t)ch, (size_t)keep * (size_t)ch * sizeof(float));
        v->sbuf_start = from;
        v->sbuf_len = keep;
        int want = STREAM_BUF - keep;
        int n = prts_stream_read(v->stream, v->sbuf + (size_t)keep * (size_t)ch, want);
        if (n < want) v->stream_ended = true;
        v->sbuf_len += n;
    }
}

/* Source frames idx and idx + 1 (zero past the end). Returns false once the source ended. */
static bool fetch(voice *v, int64_t idx, float s0[2], float s1[2])
{
    const float *a = NULL, *b = NULL;
    size_t ch = (size_t)v->channels;
    if (v->sound) {
        const prts_sound *s = v->sound;
        if (idx >= s->frames) return false;
        int64_t next = idx + 1 < s->frames ? idx + 1 : (v->loop ? 0 : -1);
        a = s->samples + (size_t)idx * ch;
        if (next >= 0) b = s->samples + (size_t)next * ch;
    } else {
        stream_need(v, idx);
        int64_t rel = idx - v->sbuf_start;
        if (rel >= v->sbuf_len) return false;
        a = v->sbuf + (size_t)rel * ch;
        if (rel + 1 < v->sbuf_len) b = a + ch;
    }
    s0[0] = a[0];
    s0[1] = a[ch - 1];
    s1[0] = b ? b[0] : 0.0f;
    s1[1] = b ? b[ch - 1] : 0.0f;
    return true;
}

static void mix_voice(prts_mixer *m, voice *v, float *out, int n)
{
    if (!v->started) {
        voice_gains(m, v, v->fade, &v->gl, &v->gr);
        v->started = true;
    }
    v->fade = approach(v->fade, v->fade_target, v->fade_rate * (float)n);
    float tl = 0.0f, tr = 0.0f;
    voice_gains(m, v, v->fade, &tl, &tr);
    float gl = v->gl, gr = v->gr;
    float dl = (tl - gl) / (float)n, dr = (tr - gr) / (float)n;
    float cutoff = prts_audio_muffle_cutoff(heard_muffle(v));
    float a = cutoff > 0.0f ? 1.0f - expf(-2.0f * (float)PRTS_PI * cutoff / (float)m->rate) : 1.0f;
    bool ended = false;
    for (int i = 0; i < n; i++) {
        int64_t idx = (int64_t)v->pos;
        float s0[2] = {0.0f, 0.0f}, s1[2] = {0.0f, 0.0f};
        if (!fetch(v, idx, s0, s1)) {
            ended = true;
            break;
        }
        float frac = (float)(v->pos - (double)idx);
        float xl = s0[0] + (s1[0] - s0[0]) * frac;
        float xr = s0[1] + (s1[1] - s0[1]) * frac;
        v->lp[0] += a * (xl - v->lp[0]);
        v->lp[1] += a * (xr - v->lp[1]);
        float yl = v->lp[0], yr = v->lp[1];
        if (v->positional) yl = yr = 0.5f * (yl + yr);
        gl += dl;
        gr += dr;
        out[2 * i] += yl * gl;
        out[2 * i + 1] += yr * gr;
        v->pos += v->step;
        if (v->sound && v->loop && v->pos >= (double)v->sound->frames) v->pos = fmod(v->pos, (double)v->sound->frames);
    }
    v->gl = tl;
    v->gr = tr;
    if (ended || (v->stop_at_end && v->fade == v->fade_target)) free_voice(v);
}

void prts_mixer_render(prts_mixer *m, float *out, int frames)
{
    memset(out, 0, (size_t)(frames > 0 ? frames : 0) * 2 * sizeof(float));
    for (int at = 0; at < frames; at += SUB_BLOCK) {
        int n = PRTS_MIN(SUB_BLOCK, frames - at);
        for (int i = 0; i < PRTS_AUDIO_MAX_VOICES; i++) {
            if (m->v[i].active) mix_voice(m, &m->v[i], out + 2 * at, n);
        }
    }
    for (int i = 0; i < frames * 2; i++) out[i] = clampf(out[i], -1.0f, 1.0f);
}

int prts_mixer_voice_count(const prts_mixer *m)
{
    int n = 0;
    for (int i = 0; i < PRTS_AUDIO_MAX_VOICES; i++) n += m->v[i].active ? 1 : 0;
    return n;
}

static void fill_info(const prts_mixer *m, const voice *v, prts_voice_info *out)
{
    out->bus = v->bus;
    out->volume = v->volume;
    out->muffle = v->muffle;
    out->occlusion = v->occlusion;
    out->fade = v->fade;
    voice_gains(m, v, v->fade, &out->gain_left, &out->gain_right);
    out->cutoff_hz = prts_audio_muffle_cutoff(heard_muffle(v));
    out->positional = v->positional;
    out->loop = v->loop;
    out->stopping = v->stop_at_end && v->fade_target == 0.0f;
    out->pos = v->at;
    out->tag = v->tag;
    out->sound = v->sound;
}

bool prts_mixer_voice_info(const prts_mixer *m, prts_voice_id id, prts_voice_info *out)
{
    const voice *v = find(m, id);
    if (v) fill_info(m, v, out);
    return v != NULL;
}

void prts_mixer_each(const prts_mixer *m, prts_mixer_each_fn fn, void *user)
{
    for (int i = 0; i < PRTS_AUDIO_MAX_VOICES; i++) {
        const voice *v = &m->v[i];
        if (!v->active) continue;
        prts_voice_info info;
        fill_info(m, v, &info);
        fn((v->gen << 8) | (uint32_t)(i + 1), &info, user);
    }
}
