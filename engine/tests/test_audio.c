/* Unit tests: sounds, WAV/OGG decoding, streaming, the mixer and the audio system. MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_audio.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "utest.h"

#define AFIX PRTS_TEST_FIXTURES "/audio"
#define RATE PRTS_AUDIO_RATE

/* --- helpers ----------------------------------------------------------------------------- */

static prts_sound *sine(double freq, float amp, int frames, int rate)
{
    prts_sound *s = prts_sound_create(frames, 1, rate);
    for (int i = 0; i < frames; i++) s->samples[i] = amp * (float)sin(2.0 * PRTS_PI * freq * i / rate);
    return s;
}

/* Power of one frequency in one channel of an interleaved stereo buffer (Goertzel). */
static double tone_power(const float *buf, int frames, int ch, double freq)
{
    double k = 2.0 * cos(2.0 * PRTS_PI * freq / RATE), s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < frames; i++) {
        double s = buf[2 * i + ch] + k * s1 - s2;
        s2 = s1;
        s1 = s;
    }
    return (s1 * s1 + s2 * s2 - k * s1 * s2) / ((double)frames * frames);
}

static double rms(const float *buf, int frames, int ch)
{
    double sum = 0.0;
    for (int i = 0; i < frames; i++) sum += (double)buf[2 * i + ch] * buf[2 * i + ch];
    return sqrt(sum / frames);
}

static prts_stream *open_fixture_stream(const char *path, bool loop)
{
    char *data = NULL;
    size_t size = 0;
    if (!prts_fs_read_file(path, &data, &size)) return NULL;
    return prts_stream_open_ogg(data, size, loop, NULL, 0);
}

static prts_voice_id play(prts_mixer *m, const prts_sound *s, prts_audio_bus bus, bool positional, prts_dvec3 at)
{
    prts_voice_params p;
    prts_voice_params_default(&p);
    p.sound = s;
    p.bus = bus;
    p.loop = true;
    p.positional = positional;
    p.pos = at;
    return prts_mixer_play(m, &p);
}

/* --- spatial maths ------------------------------------------------------------------------ */

UTEST(audio, buses_pan_attenuation_and_muffle_curve)
{
    prts_audio_bus b = PRTS_BUS_MASTER;
    ASSERT_TRUE(prts_audio_bus_parse("ambience", &b));
    ASSERT_EQ(PRTS_BUS_AMBIENCE, b);
    ASSERT_FALSE(prts_audio_bus_parse("loud", &b));
    ASSERT_FALSE(prts_audio_bus_parse(NULL, &b));
    ASSERT_STREQ("music", prts_audio_bus_name(PRTS_BUS_MUSIC));
    ASSERT_STREQ("?", prts_audio_bus_name(PRTS_BUS_COUNT));

    /* equal-power panning: constant total power, hard left/right at the ends */
    float l, r;
    for (float pan = -1.0f; pan <= 1.0f; pan += 0.25f) {
        prts_audio_pan_gains(pan, &l, &r);
        ASSERT_NEAR(1.0f, l * l + r * r, 1e-5f);
    }
    prts_audio_pan_gains(-1.0f, &l, &r);
    ASSERT_NEAR(1.0f, l, 1e-6f);
    ASSERT_NEAR(0.0f, r, 1e-6f);
    prts_audio_pan_gains(0.0f, &l, &r);
    ASSERT_NEAR(l, r, 1e-6f);
    prts_audio_pan_gains(7.0f, &l, &r); /* clamped */
    ASSERT_NEAR(1.0f, r, 1e-6f);
    prts_audio_pan_gains(NAN, &l, &r);
    ASSERT_NEAR(1.0f, l, 1e-6f);

    ASSERT_EQ(1.0f, prts_audio_attenuation(0.5f, 2.0f, 30.0f));
    ASSERT_EQ(1.0f, prts_audio_attenuation(2.0f, 2.0f, 30.0f));
    ASSERT_EQ(0.0f, prts_audio_attenuation(30.0f, 2.0f, 30.0f));
    ASSERT_EQ(0.0f, prts_audio_attenuation(NAN, 2.0f, 30.0f));
    float prev = 1.0f;
    for (float d = 2.5f; d < 30.0f; d += 2.5f) {
        float a = prts_audio_attenuation(d, 2.0f, 30.0f);
        ASSERT_LT(a, prev);
        prev = a;
    }
    /* defaults: 1.5 m reference distance, silent 40 m beyond it */
    ASSERT_EQ(1.0f, prts_audio_attenuation(1.5f, 0.0f, 0.0f));
    ASSERT_GT(prts_audio_attenuation(41.0f, 0.0f, 0.0f), 0.0f);
    ASSERT_EQ(0.0f, prts_audio_attenuation(41.5f, 0.0f, 0.0f));

    ASSERT_EQ(0.0f, prts_audio_muffle_cutoff(0.0f));
    ASSERT_EQ(0.0f, prts_audio_muffle_cutoff(-1.0f));
    ASSERT_NEAR(350.0f, prts_audio_muffle_cutoff(1.0f), 0.5f);
    ASSERT_NEAR(350.0f, prts_audio_muffle_cutoff(3.0f), 0.5f);
    ASSERT_GT(prts_audio_muffle_cutoff(0.25f), prts_audio_muffle_cutoff(0.75f));
}

/* --- the mixer on rendered buffers --------------------------------------------------------- */

UTEST(audio, sound_to_the_left)
{
    /* Spec: Sound to the left (a door to the commander's left is louder on the left) */
    enum { N = 4800 };
    static float out[N * 2];
    prts_sound *s = sine(440.0, 0.5f, 24000, 24000);
    prts_mixer *m = prts_mixer_create(RATE);
    ASSERT_EQ(RATE, prts_mixer_rate(m));
    prts_voice_id v = play(m, s, PRTS_BUS_EFFECTS, true, prts_dv3(-4, 1.7, 0));
    ASSERT_NE(PRTS_VOICE_NONE, v);
    prts_mixer_set_listener(m, prts_dv3(0, 1.7, 0), 0.0f); /* looking along -z: -x is left */
    prts_mixer_render(m, out, N);
    ASSERT_GT(rms(out, N, 0), 10.0 * rms(out, N, 1));
    /* turned to look along -x, the same door is ahead: both sides the same */
    prts_mixer_set_listener(m, prts_dv3(0, 1.7, 0), (float)(PRTS_PI / 2));
    prts_mixer_render(m, out, N); /* the change is ramped over the first block */
    prts_mixer_render(m, out, N);
    ASSERT_NEAR(rms(out, N, 0), rms(out, N, 1), 1e-4);
    /* turned to look along +z, it is on the right */
    prts_mixer_set_listener(m, prts_dv3(0, 1.7, 0), (float)PRTS_PI);
    prts_mixer_render(m, out, N);
    prts_mixer_render(m, out, N);
    ASSERT_GT(rms(out, N, 1), 10.0 * rms(out, N, 0));
    /* moving the sound far away makes it silent */
    ASSERT_TRUE(prts_mixer_set_position(m, v, prts_dv3(0, 1.7, -500)));
    prts_mixer_render(m, out, N);
    prts_mixer_render(m, out, N);
    ASSERT_EQ(0.0, rms(out, N, 0) + rms(out, N, 1));
    prts_mixer_destroy(m);
    prts_sound_destroy(s);
}

UTEST(audio, muting_music)
{
    /* Spec: Muting music (music silent, effects still audible) */
    enum { N = 4800 };
    static float out[N * 2];
    prts_sound *tune = sine(440.0, 0.4f, 48000, RATE), *fx = sine(1000.0, 0.4f, 48000, RATE);
    prts_mixer *m = prts_mixer_create(RATE);
    prts_voice_id music = play(m, tune, PRTS_BUS_MUSIC, false, prts_dv3(0, 0, 0));
    play(m, fx, PRTS_BUS_EFFECTS, false, prts_dv3(0, 0, 0));
    prts_mixer_render(m, out, N);
    double music_before = tone_power(out, N, 0, 440.0), fx_before = tone_power(out, N, 0, 1000.0);
    ASSERT_GT(music_before, 0.01);
    ASSERT_GT(fx_before, 0.01);
    prts_mixer_set_bus_volume(m, PRTS_BUS_MUSIC, 0.0f);
    ASSERT_EQ(0.0f, prts_mixer_bus_volume(m, PRTS_BUS_MUSIC));
    prts_mixer_render(m, out, N); /* ramps down without a click */
    prts_mixer_render(m, out, N);
    ASSERT_LT(tone_power(out, N, 0, 440.0), music_before * 1e-6);
    ASSERT_NEAR(fx_before, tone_power(out, N, 0, 1000.0), fx_before * 0.05);
    prts_voice_info info;
    ASSERT_TRUE(prts_mixer_voice_info(m, music, &info));
    ASSERT_EQ(0.0f, info.gain_left);
    /* the master bus silences everything; volumes are clamped to 0..1 */
    prts_mixer_set_bus_volume(m, PRTS_BUS_MUSIC, 4.0f);
    ASSERT_EQ(1.0f, prts_mixer_bus_volume(m, PRTS_BUS_MUSIC));
    prts_mixer_set_bus_volume(m, PRTS_BUS_MASTER, 0.0f);
    prts_mixer_set_bus_volume(m, PRTS_BUS_COUNT, 0.5f);
    ASSERT_EQ(0.0f, prts_mixer_bus_volume(m, PRTS_BUS_COUNT));
    prts_mixer_render(m, out, N);
    prts_mixer_render(m, out, N);
    ASSERT_EQ(0.0, rms(out, N, 0));
    prts_mixer_destroy(m);
    prts_sound_destroy(tune);
    prts_sound_destroy(fx);
}

UTEST(audio, muffling_takes_high_frequencies_away)
{
    /* Spec: Door closes on a noisy room (quieter, and high frequencies lose energy) */
    enum { N = 9600 };
    static float out[N * 2];
    prts_sound *s = prts_sound_create(48000, 1, RATE);
    for (int i = 0; i < s->frames; i++) {
        s->samples[i] = 0.3f * (float)sin(2.0 * PRTS_PI * 200.0 * i / RATE) + 0.3f * (float)sin(2.0 * PRTS_PI * 6000.0 * i / RATE);
    }
    prts_mixer *m = prts_mixer_create(RATE);
    prts_voice_id v = play(m, s, PRTS_BUS_AMBIENCE, true, prts_dv3(0, 0, -1));
    prts_mixer_render(m, out, N);
    double low_clear = tone_power(out, N, 0, 200.0), high_clear = tone_power(out, N, 0, 6000.0);
    double rms_clear = rms(out, N, 0);
    ASSERT_TRUE(prts_mixer_set_muffle(m, v, 0.5f));
    prts_mixer_render(m, out, N);
    prts_mixer_render(m, out, N);
    double high_half = tone_power(out, N, 0, 6000.0);
    ASSERT_TRUE(prts_mixer_set_muffle(m, v, 1.0f));
    prts_mixer_render(m, out, N);
    prts_mixer_render(m, out, N);
    double low_muffled = tone_power(out, N, 0, 200.0), high_muffled = tone_power(out, N, 0, 6000.0);
    ASSERT_LT(rms(out, N, 0), rms_clear * 0.5);  /* quieter */
    ASSERT_LT(high_muffled, high_clear * 0.002); /* the highs are nearly gone */
    ASSERT_LT(high_muffled, high_half);
    ASSERT_LT(high_half, high_clear);
    ASSERT_GT(low_muffled, low_clear * 0.05);    /* the lows survive */
    ASSERT_GT(low_muffled / low_clear, 20.0 * high_muffled / high_clear);
    prts_voice_info info;
    ASSERT_TRUE(prts_mixer_voice_info(m, v, &info));
    ASSERT_EQ(1.0f, info.muffle);
    ASSERT_NEAR(350.0f, info.cutoff_hz, 0.5f);
    /* opening the door makes it clear again */
    ASSERT_TRUE(prts_mixer_set_muffle(m, v, 0.0f));
    prts_mixer_render(m, out, N);
    prts_mixer_render(m, out, N);
    ASSERT_NEAR(high_clear, tone_power(out, N, 0, 6000.0), high_clear * 0.05);
    /* the engine's occlusion (closed portals) and the script's muffle: the stronger one counts */
    ASSERT_TRUE(prts_mixer_set_occlusion(m, v, 1.0f));
    ASSERT_TRUE(prts_mixer_set_muffle(m, v, 0.25f));
    ASSERT_TRUE(prts_mixer_voice_info(m, v, &info));
    ASSERT_EQ(1.0f, info.occlusion);
    ASSERT_NEAR(350.0f, info.cutoff_hz, 0.5f);
    ASSERT_TRUE(prts_mixer_set_occlusion(m, v, -3.0f)); /* clamped to 0 */
    ASSERT_TRUE(prts_mixer_voice_info(m, v, &info));
    ASSERT_EQ(0.0f, info.occlusion);
    ASSERT_NEAR(prts_audio_muffle_cutoff(0.25f), info.cutoff_hz, 0.5f);
    ASSERT_FALSE(prts_mixer_set_occlusion(m, PRTS_VOICE_NONE, 1.0f));
    prts_mixer_destroy(m);
    prts_sound_destroy(s);
}

UTEST(audio, cross_fade_between_streams)
{
    /* Spec: Cross-fade (menu track out, Proving Ground track in over 2 s, no gap or click) */
    enum { BLOCK = 480 };
    static float out[BLOCK * 2];
    prts_mixer *m = prts_mixer_create(RATE);
    prts_voice_params p;
    prts_voice_params_default(&p);
    p.bus = PRTS_BUS_MUSIC;
    p.stream = open_fixture_stream(AFIX "/tone_440.ogg", true);
    ASSERT_TRUE(p.stream != NULL);
    prts_voice_id a = prts_mixer_play(m, &p);
    for (int i = 0; i < 50; i++) prts_mixer_render(m, out, BLOCK); /* 0.5 s */
    ASSERT_GT(rms(out, BLOCK, 0), 0.3);
    ASSERT_TRUE(prts_mixer_fade(m, a, 0.0f, 2.0f, true));
    p.stream = open_fixture_stream(AFIX "/tone_660_stereo.ogg", true);
    p.fade_in = 2.0f;
    prts_voice_id b = prts_mixer_play(m, &p);
    ASSERT_NE(PRTS_VOICE_NONE, b);
    float last = out[2 * (BLOCK - 1)], max_step = 0.0f, prev_a = 1.0f, prev_b = 0.0f;
    double min_rms = 1.0;
    for (int blk = 0; blk < 220; blk++) { /* 2.2 s */
        prts_mixer_render(m, out, BLOCK);
        for (int i = 0; i < BLOCK; i++) {
            max_step = fmaxf(max_step, fabsf(out[2 * i] - last));
            last = out[2 * i];
        }
        min_rms = fmin(min_rms, rms(out, BLOCK, 0));
        prts_voice_info ia, ib;
        ASSERT_TRUE(prts_mixer_voice_info(m, b, &ib));
        ASSERT_GE(ib.fade, prev_b);
        prev_b = ib.fade;
        if (prts_mixer_voice_info(m, a, &ia)) {
            ASSERT_LE(ia.fade, prev_a);
            ASSERT_TRUE(ia.stopping);
            ASSERT_NEAR(1.0f, ia.fade + ib.fade, 0.02f);
            prev_a = ia.fade;
        }
    }
    ASSERT_GT(min_rms, 0.3);   /* no gap, not even a dip (equal-power fades) */
    ASSERT_LT(max_step, 0.1f); /* no click (a 440/660 Hz tone moves < 0.05 per sample) */
    ASSERT_FALSE(prts_mixer_playing(m, a));
    ASSERT_EQ(1.0f, prev_b);
    ASSERT_EQ(1, prts_mixer_voice_count(m));
    /* the stereo track keeps its channels: left louder than right */
    prts_mixer_render(m, out, BLOCK);
    ASSERT_GT(rms(out, BLOCK, 0), 1.5 * rms(out, BLOCK, 1));
    prts_mixer_destroy(m);
}

typedef struct each_ctx {
    int count;
    uint64_t tags;
} each_ctx;

static void count_voice(prts_voice_id v, const prts_voice_info *info, void *user)
{
    each_ctx *c = user;
    c->count += v != PRTS_VOICE_NONE;
    c->tags += info->tag;
}

UTEST(audio, voices_handles_and_lifetimes)
{
    enum { N = 2048 };
    static float out[N * 2];
    prts_mixer *m = prts_mixer_create(0);
    ASSERT_EQ(RATE, prts_mixer_rate(m));
    prts_sound *s = sine(500.0, 0.5f, 1000, 24000); /* 1000 frames at half rate: 2000 output frames */
    prts_voice_params p;
    prts_voice_params_default(&p);
    p.sound = s;
    p.tag = 7;
    p.bus = (prts_audio_bus)99; /* unknown buses become effects */
    prts_voice_id once = prts_mixer_play(m, &p);
    prts_voice_info info;
    ASSERT_TRUE(prts_mixer_voice_info(m, once, &info));
    ASSERT_EQ(PRTS_BUS_EFFECTS, info.bus);
    ASSERT_EQ((uint64_t)7, info.tag);
    ASSERT_FALSE(info.loop);
    prts_mixer_render(m, out, 1900);
    ASSERT_TRUE(prts_mixer_playing(m, once));
    prts_mixer_render(m, out, 200); /* the sound ends and its voice is freed */
    ASSERT_FALSE(prts_mixer_playing(m, once));
    ASSERT_EQ(0.0f, out[2 * 150]);
    /* stale handles are refused everywhere */
    ASSERT_FALSE(prts_mixer_stop(m, once, 0.1f));
    ASSERT_FALSE(prts_mixer_fade(m, once, 0.5f, 1.0f, false));
    ASSERT_FALSE(prts_mixer_set_volume(m, once, 0.5f));
    ASSERT_FALSE(prts_mixer_set_position(m, once, prts_dv3(0, 0, 0)));
    ASSERT_FALSE(prts_mixer_set_muffle(m, once, 0.5f));
    ASSERT_FALSE(prts_mixer_voice_info(m, once, &info));
    ASSERT_FALSE(prts_mixer_playing(m, PRTS_VOICE_NONE));
    ASSERT_FALSE(prts_mixer_playing(m, 0xff));
    /* the slot is reused with a new generation */
    p.loop = true;
    p.fade_in = 0.01f;
    p.volume = 99.0f;
    prts_voice_id loop = prts_mixer_play(m, &p);
    ASSERT_NE(once, loop);
    ASSERT_EQ(once & 0xffu, loop & 0xffu);
    ASSERT_TRUE(prts_mixer_voice_info(m, loop, &info));
    ASSERT_EQ(8.0f, info.volume);
    ASSERT_EQ(0.0f, info.fade);
    ASSERT_TRUE(prts_mixer_set_volume(m, loop, 0.5f));
    for (int i = 0; i < 4; i++) prts_mixer_render(m, out, N); /* loops past its end */
    ASSERT_TRUE(prts_mixer_playing(m, loop));
    ASSERT_GT(rms(out, N, 0), 0.1);
    /* fading to a level without stopping, then to silence with stopping */
    ASSERT_TRUE(prts_mixer_fade(m, loop, 0.5f, 0.0f, false));
    prts_mixer_render(m, out, N);
    ASSERT_TRUE(prts_mixer_voice_info(m, loop, &info));
    ASSERT_EQ(0.5f, info.fade);
    ASSERT_FALSE(info.stopping);
    ASSERT_TRUE(prts_mixer_fade(m, loop, 0.25f, 0.01f, true));
    prts_mixer_render(m, out, N);
    ASSERT_FALSE(prts_mixer_playing(m, loop));
    /* stop fades out quickly; a second, slower stop does not slow it down */
    loop = prts_mixer_play(m, &p);
    ASSERT_TRUE(prts_mixer_stop(m, loop, 0.0f));
    ASSERT_TRUE(prts_mixer_stop(m, loop, 10.0f));
    prts_mixer_render(m, out, 512);
    ASSERT_FALSE(prts_mixer_playing(m, loop));
    /* every voice busy: play refuses, and a stream handed over is released */
    for (int i = 0; i < PRTS_AUDIO_MAX_VOICES; i++) ASSERT_NE(PRTS_VOICE_NONE, prts_mixer_play(m, &p));
    ASSERT_EQ(PRTS_AUDIO_MAX_VOICES, prts_mixer_voice_count(m));
    ASSERT_EQ(PRTS_VOICE_NONE, prts_mixer_play(m, &p));
    prts_voice_params q;
    prts_voice_params_default(&q);
    q.stream = open_fixture_stream(AFIX "/tone_440.ogg", true);
    ASSERT_EQ(PRTS_VOICE_NONE, prts_mixer_play(m, &q));
    each_ctx c = {0, 0};
    prts_mixer_each(m, count_voice, &c);
    ASSERT_EQ(PRTS_AUDIO_MAX_VOICES, c.count);
    ASSERT_EQ((uint64_t)7 * PRTS_AUDIO_MAX_VOICES, c.tags);
    prts_mixer_stop_all(m);
    ASSERT_EQ(0, prts_mixer_voice_count(m));
    /* nothing to play: no sound, an empty sound, too many channels; a sound wins over a stream */
    prts_voice_params_default(&q);
    ASSERT_EQ(PRTS_VOICE_NONE, prts_mixer_play(m, &q));
    prts_sound *empty = prts_sound_create(0, 1, RATE), *wide = prts_sound_create(10, 3, RATE);
    q.sound = empty;
    ASSERT_EQ(PRTS_VOICE_NONE, prts_mixer_play(m, &q));
    q.sound = wide;
    ASSERT_EQ(PRTS_VOICE_NONE, prts_mixer_play(m, &q));
    q.sound = s;
    q.stream = open_fixture_stream(AFIX "/tone_440.ogg", true);
    prts_voice_id both = prts_mixer_play(m, &q);
    ASSERT_TRUE(prts_mixer_voice_info(m, both, &info));
    prts_mixer_render(m, out, 0);
    prts_mixer_destroy(m);
    prts_mixer_destroy(NULL);
    prts_sound_destroy(s);
    prts_sound_destroy(empty);
    prts_sound_destroy(wide);
    prts_sound_destroy(NULL);
}

UTEST(audio, a_stream_that_ends)
{
    enum { N = 4096 };
    static float out[N * 2];
    prts_mixer *m = prts_mixer_create(RATE);
    prts_voice_params p;
    prts_voice_params_default(&p);
    p.stream = open_fixture_stream(AFIX "/tone_440.ogg", false);
    prts_voice_id v = prts_mixer_play(m, &p);
    /* 2.5 s at 22050 Hz is about 5.44 s worth of frames... at 48 kHz: 120000 output frames */
    int rendered = 0;
    while (prts_mixer_playing(m, v) && rendered < RATE * 4) {
        prts_mixer_render(m, out, N);
        rendered += N;
    }
    ASSERT_FALSE(prts_mixer_playing(m, v));
    ASSERT_NEAR(2.5 * RATE, (double)rendered, N + 10.0);
    prts_mixer_destroy(m);
}

/* --- decoding ---------------------------------------------------------------------------- */

static prts_sound *decode_file(const char *path, char *err)
{
    char *data = NULL;
    size_t size = 0;
    if (!prts_fs_read_file(path, &data, &size)) return NULL;
    prts_sound *s = prts_sound_decode(data, size, err, 128);
    prts_free(data);
    return s;
}

UTEST(audio, wav_and_ogg_fixtures)
{
    char err[128];
    prts_sound *s = decode_file(AFIX "/tone_16bit.wav", err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(1, s->channels);
    ASSERT_EQ(22050, s->rate);
    ASSERT_EQ(5512, s->frames);
    float peak = 0.0f;
    for (int i = 0; i < s->frames; i++) peak = fmaxf(peak, fabsf(s->samples[i]));
    ASSERT_NEAR(0.5f, peak, 0.01f);
    prts_sound_destroy(s);

    s = decode_file(AFIX "/left_8bit_stereo.wav", err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(2, s->channels);
    ASSERT_EQ(11025, s->rate);
    float left = 0.0f, right = 0.0f;
    for (int i = 0; i < s->frames; i++) {
        left = fmaxf(left, fabsf(s->samples[2 * i]));
        right = fmaxf(right, fabsf(s->samples[2 * i + 1]));
    }
    ASSERT_NEAR(0.5f, left, 0.02f);
    ASSERT_EQ(0.0f, right);
    prts_sound_destroy(s);

    s = decode_file(AFIX "/tone_440.ogg", err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(1, s->channels);
    ASSERT_EQ(22050, s->rate);
    ASSERT_NEAR(55125.0, (double)s->frames, 1100.0);
    prts_sound_destroy(s);

    s = decode_file(AFIX "/tone_660_stereo.ogg", err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(2, s->channels);
    prts_sound_destroy(s);

    /* a valid stream without audio, and broken Ogg data */
    ASSERT_TRUE(decode_file(AFIX "/empty.ogg", err) == NULL);
    ASSERT_TRUE(strstr(err, "no samples") != NULL);
    const char junk[] = "OggS this is not a vorbis stream at all";
    ASSERT_TRUE(prts_sound_decode(junk, sizeof(junk), err, sizeof(err)) == NULL);
    ASSERT_TRUE(strstr(err, "OGG") != NULL);
    char *copy = prts_strdup(junk);
    ASSERT_TRUE(prts_stream_open_ogg(copy, sizeof(junk), true, err, sizeof(err)) == NULL); /* frees copy */
    ASSERT_TRUE(prts_sound_decode_ogg(junk, sizeof(junk), NULL, 0) == NULL);
}

/* Builds a WAV file in memory. fmt_size 16 is plain, 40 is WAVE_FORMAT_EXTENSIBLE. */
static size_t make_wav(unsigned char *buf, int format, int channels, int rate, int bits, int sub_format,
                       int fmt_size, const void *data, uint32_t data_len, uint32_t claimed_len)
{
    size_t n = 0;
#define PUT(p, len) (memcpy(buf + n, (p), (len)), n += (len))
#define U16(v) do { uint16_t x_ = (uint16_t)(v); unsigned char b_[2] = {(unsigned char)x_, (unsigned char)(x_ >> 8)}; PUT(b_, 2); } while (0)
#define U32(v) do { uint32_t x_ = (uint32_t)(v); unsigned char b_[4] = {(unsigned char)x_, (unsigned char)(x_ >> 8), (unsigned char)(x_ >> 16), (unsigned char)(x_ >> 24)}; PUT(b_, 4); } while (0)
    PUT("RIFF", 4);
    U32(0);
    PUT("WAVE", 4);
    PUT("LIST", 4); /* an odd-sized chunk to skip, with its pad byte */
    U32(3);
    PUT("abc\0", 4);
    if (fmt_size > 0) {
        PUT("fmt ", 4);
        U32(fmt_size);
        U16(format);
        U16(channels);
        U32(rate);
        U32(rate * channels * bits / 8);
        U16(channels * bits / 8);
        U16(bits);
        if (fmt_size >= 18) U16(fmt_size - 18);
        if (fmt_size >= 26) {
            U16(bits);
            U32(3);
            U16(sub_format);
        }
        for (int i = 26; i < fmt_size; i++) PUT("\0", 1);
    }
    PUT("data", 4);
    U32(claimed_len);
    if (data_len) PUT(data, data_len);
    return n;
#undef PUT
#undef U16
#undef U32
}

static prts_sound *wav(int format, int channels, int bits, int sub_format, int fmt_size, const void *data,
                       uint32_t len, char *err)
{
    static unsigned char buf[512];
    size_t n = make_wav(buf, format, channels, 22050, bits, sub_format, fmt_size, data, len, len);
    return prts_sound_decode_wav(buf, n, err, 128);
}

UTEST(audio, wav_sample_formats_and_errors)
{
    char err[128];
    /* 24-bit: -1, ~+1, 0.5 */
    const unsigned char s24[] = {0x00, 0x00, 0x80, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x40};
    prts_sound *s = wav(1, 1, 24, 0, 16, s24, sizeof(s24), err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(3, s->frames);
    ASSERT_NEAR(-1.0f, s->samples[0], 1e-6f);
    ASSERT_NEAR(1.0f, s->samples[1], 1e-6f);
    ASSERT_NEAR(0.5f, s->samples[2], 1e-6f);
    prts_sound_destroy(s);
    /* 32-bit integer and 16-bit negative values */
    const unsigned char s32[] = {0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40};
    s = wav(1, 2, 32, 0, 16, s32, sizeof(s32), err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(1, s->frames);
    ASSERT_NEAR(-1.0f, s->samples[0], 1e-6f);
    ASSERT_NEAR(0.5f, s->samples[1], 1e-6f);
    prts_sound_destroy(s);
    const unsigned char s16[] = {0x00, 0x80, 0x00, 0x40};
    s = wav(1, 1, 16, 0, 16, s16, sizeof(s16), err);
    ASSERT_NEAR(-1.0f, s->samples[0], 1e-6f);
    ASSERT_NEAR(0.5f, s->samples[1], 1e-6f);
    prts_sound_destroy(s);
    /* float 32 and 64 */
    const float f32[] = {0.25f, -0.75f};
    s = wav(3, 1, 32, 0, 16, f32, sizeof(f32), err);
    ASSERT_EQ(-0.75f, s->samples[1]);
    prts_sound_destroy(s);
    const double f64[] = {0.125, -0.5};
    s = wav(3, 2, 64, 0, 18, f64, sizeof(f64), err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(0.125f, s->samples[0]);
    prts_sound_destroy(s);
    /* WAVE_FORMAT_EXTENSIBLE with PCM and float sub-formats */
    s = wav(0xFFFE, 1, 16, 1, 40, s16, sizeof(s16), err);
    ASSERT_TRUE(s != NULL);
    prts_sound_destroy(s);
    s = wav(0xFFFE, 1, 32, 3, 40, f32, sizeof(f32), err);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(0.25f, s->samples[0]);
    prts_sound_destroy(s);

    /* refused */
    ASSERT_TRUE(wav(0xFFFE, 1, 16, 2, 40, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(strstr(err, "PCM") != NULL);
    ASSERT_TRUE(wav(0xFFFE, 1, 16, 1, 22, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(strstr(err, "extensible") != NULL);
    ASSERT_TRUE(wav(2, 1, 4, 0, 16, s16, sizeof(s16), err) == NULL); /* ADPCM */
    ASSERT_TRUE(wav(1, 3, 16, 0, 16, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(strstr(err, "mono or stereo") != NULL);
    ASSERT_TRUE(wav(1, 0, 16, 0, 16, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(wav(1, 1, 12, 0, 16, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(strstr(err, "sample size") != NULL);
    ASSERT_TRUE(wav(3, 1, 16, 0, 16, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(wav(1, 1, 16, 0, 14, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(strstr(err, "too short") != NULL);
    ASSERT_TRUE(wav(1, 1, 16, 0, 0, s16, sizeof(s16), err) == NULL);
    ASSERT_TRUE(strstr(err, "no fmt chunk") != NULL);
    ASSERT_TRUE(wav(1, 1, 16, 0, 16, s16, 1, err) == NULL);
    ASSERT_TRUE(strstr(err, "no samples") != NULL);
    unsigned char buf[512];
    size_t n = make_wav(buf, 1, 1, 500, 16, 0, 16, s16, sizeof(s16), sizeof(s16));
    ASSERT_TRUE(prts_sound_decode_wav(buf, n, err, sizeof(err)) == NULL);
    ASSERT_TRUE(strstr(err, "rate") != NULL);
    /* a data length past the end of the file is tolerated */
    n = make_wav(buf, 1, 1, 22050, 16, 0, 16, s16, sizeof(s16), 0xffffffffu);
    s = prts_sound_decode_wav(buf, n, err, sizeof(err));
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(2, s->frames);
    prts_sound_destroy(s);
    /* no data chunk at all, or a chunk running past the end */
    n = make_wav(buf, 1, 1, 22050, 16, 0, 16, NULL, 0, 0) - 8;
    ASSERT_TRUE(prts_sound_decode_wav(buf, n, err, sizeof(err)) == NULL);
    ASSERT_TRUE(strstr(err, "no data chunk") != NULL);
    memcpy(buf + n, "junk", 4);
    buf[n + 4] = 0xff;
    buf[n + 5] = buf[n + 6] = buf[n + 7] = 0x0f;
    ASSERT_TRUE(prts_sound_decode_wav(buf, n + 8, err, sizeof(err)) == NULL);
    ASSERT_TRUE(prts_sound_decode("RIFF", 4, err, sizeof(err)) == NULL);
    ASSERT_TRUE(strstr(err, "RIFF") != NULL);
    ASSERT_TRUE(prts_sound_decode_wav("RIFFxxxxWAVX", 12, NULL, 0) == NULL);

    /* the placeholder beep is short and starts and ends quietly */
    prts_sound *beep = prts_sound_beep(RATE);
    ASSERT_EQ(RATE * 15 / 100, beep->frames);
    ASSERT_EQ(0.0f, beep->samples[0]);
    ASSERT_LT(fabsf(beep->samples[beep->frames - 1]), 1e-3f);
    prts_sound_destroy(beep);
}

UTEST(audio, streams_loop_seamlessly_and_end)
{
    enum { N = 8000 };
    static float buf[N * 2];
    prts_stream *s = open_fixture_stream(AFIX "/tone_660_stereo.ogg", true);
    ASSERT_TRUE(s != NULL);
    ASSERT_EQ(2, prts_stream_channels(s));
    ASSERT_EQ(22050, prts_stream_rate(s));
    int total = 0;
    for (int i = 0; i < 20; i++) total += prts_stream_read(s, buf, N); /* 160000 frames: wraps twice */
    ASSERT_EQ(20 * N, total);
    prts_stream_destroy(s);
    prts_stream_destroy(NULL);
    s = open_fixture_stream(AFIX "/tone_440.ogg", false);
    ASSERT_EQ(1, prts_stream_channels(s));
    total = 0;
    int n;
    while ((n = prts_stream_read(s, buf, N)) > 0) total += n;
    ASSERT_NEAR(55125.0, (double)total, 1100.0);
    ASSERT_EQ(0, prts_stream_read(s, buf, N));
    prts_stream_destroy(s);
}

/* --- the audio system: cache, music, null and SDL devices ------------------------------------ */

UTEST(audio, system_cache_music_and_null_device)
{
    prts_vfs *vfs = prts_vfs_create();
    ASSERT_TRUE(prts_vfs_mount(vfs, AFIX, "fixtures"));
    prts_audio *a = prts_audio_create(vfs, false, NULL);
    ASSERT_FALSE(prts_audio_has_device(a));
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_log_reset_error_count();
    const prts_sound *tone = prts_audio_sound(a, "tone_16bit.wav");
    ASSERT_EQ(tone, prts_audio_sound(a, "tone_16bit.wav"));
    const prts_sound *beep = prts_audio_sound(a, "missing.wav");
    ASSERT_EQ(beep, prts_audio_sound(a, "missing.wav")); /* logged once */
    ASSERT_NE(beep, prts_audio_sound(a, "tone_440.ogg")); /* OGG works for effects too */
    ASSERT_EQ((size_t)1, prts_log_error_count());
    prts_voice_params p;
    prts_voice_params_default(&p);
    p.stream = (prts_stream *)1; /* ignored by prts_audio_play */
    prts_voice_id v = prts_audio_play(a, "tone_16bit.wav", &p);
    ASSERT_TRUE(prts_mixer_playing(prts_audio_mixer(a), v));
    ASSERT_TRUE(prts_audio_set_muffle(a, v, 0.3f));
    prts_audio_set_listener(a, prts_dv3(1, 2, 3), 0.5f);

    /* music: start, keep, cross-fade, beep for broken tracks, stop */
    prts_voice_id m1 = prts_audio_music(a, "tone_440.ogg", 0.0f, 1.0f);
    ASSERT_EQ(m1, prts_audio_music_voice(a));
    ASSERT_EQ(m1, prts_audio_music(a, "tone_440.ogg", 1.0f, 0.5f));
    prts_voice_id m2 = prts_audio_music(a, "tone_660_stereo.ogg", 1.0f, 1.0f);
    ASSERT_NE(m1, m2);
    prts_voice_info info;
    ASSERT_TRUE(prts_mixer_voice_info(prts_audio_mixer(a), m1, &info));
    ASSERT_TRUE(info.stopping);
    ASSERT_TRUE(prts_mixer_voice_info(prts_audio_mixer(a), m2, &info));
    ASSERT_EQ(PRTS_BUS_MUSIC, info.bus);
    ASSERT_TRUE(info.loop);
    ASSERT_NE(PRTS_VOICE_NONE, prts_audio_music(a, "missing.ogg", 0.5f, 1.0f));
    ASSERT_NE(PRTS_VOICE_NONE, prts_audio_music(a, "tone_16bit.wav", 0.5f, 1.0f));
    ASSERT_EQ((size_t)3, prts_log_error_count());
    ASSERT_EQ(PRTS_VOICE_NONE, prts_audio_music(a, NULL, 0.5f, 1.0f));
    ASSERT_EQ(PRTS_VOICE_NONE, prts_audio_music_voice(a));
    prts_log_set_min_level(PRTS_LOG_INFO);

    /* the null device consumes audio as time passes, so levels follow what plays */
    float l = 1.0f, r = 1.0f;
    prts_audio_levels(a, &l, &r);
    ASSERT_EQ(0.0f, l);
    prts_audio_update(a, 0.0);
    prts_audio_update(a, -1.0);
    prts_audio_levels(a, &l, &r);
    ASSERT_EQ(0.0f, l);
    prts_audio_update(a, 1.0 / 30.0);
    prts_audio_levels(a, &l, &r);
    ASSERT_GT(l, 0.01f);
    prts_audio_update(a, 100.0); /* long stalls are capped */
    prts_audio_resume(a);
    prts_audio_destroy(a);
    prts_audio_destroy(NULL);
    prts_vfs_destroy(vfs);
}

UTEST(audio, sdl_device_smoke_test)
{
    /* SDL's dummy driver stands in for a real device on machines without sound */
    prts_vfs *vfs = prts_vfs_create();
    prts_audio *a = prts_audio_create(vfs, true, "dummy");
    ASSERT_TRUE(prts_audio_has_device(a));
    prts_voice_params p;
    prts_voice_params_default(&p);
    prts_audio_play(a, "missing.wav", &p);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_audio_update(a, 0.0); /* fills the queue... */
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_audio_device *d = prts_audio_device_open(RATE, "dummy");
    ASSERT_TRUE(d != NULL);
    float frames[64] = {0};
    ASSERT_TRUE(prts_audio_device_write(d, frames, 32));
    ASSERT_FALSE(prts_audio_device_write(d, frames, 0));
    ASSERT_GT(prts_audio_device_queued(d), 0);
    prts_audio_device_resume(d);
    prts_audio_device_close(d);
    prts_audio_device_close(NULL);
    prts_audio_update(a, 0.0); /* ...and tops it up */
    prts_audio_resume(a);
    float l, r;
    prts_audio_levels(a, &l, &r);
    prts_audio_destroy(a);
    /* an unknown driver falls back to the null device */
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    a = prts_audio_create(vfs, true, "no_such_driver");
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_FALSE(prts_audio_has_device(a));
    prts_audio_destroy(a);
    prts_vfs_destroy(vfs);
}

UTEST(audio, start_requests_wait_until_the_device_runs)
{
    /* Spec: Browser audio unlock. A browser lets nothing play before the first user gesture,
     * yet takes queued audio away meanwhile, so the audio system mixes nothing until the
     * device runs: sounds and music started before then wait at their first sample. The
     * paused SDL dummy device stands in for the browser's suspended AudioContext. */
    prts_vfs *vfs = prts_vfs_create();
    ASSERT_TRUE(prts_vfs_mount(vfs, AFIX, "fixtures"));
    prts_audio *a = prts_audio_create(vfs, true, "dummy");
    ASSERT_TRUE(prts_audio_has_device(a));
    prts_mixer *m = prts_audio_mixer(a);
    prts_audio_pause(a);
    prts_voice_id music = prts_audio_music(a, "tone_440.ogg", 1.0f, 1.0f); /* the menu music, at boot */
    prts_voice_params p;
    prts_voice_params_default(&p);
    prts_voice_id sound = prts_audio_play(a, "tone_16bit.wav", &p);
    for (int i = 0; i < 60; i++) prts_audio_update(a, 1.0 / 30.0); /* two seconds before the click */
    ASSERT_TRUE(prts_audio_held(a));
    ASSERT_EQ((uint64_t)0, prts_audio_frames_mixed(a));
    float l = 1.0f, r = 1.0f;
    prts_audio_levels(a, &l, &r);
    ASSERT_EQ(0.0f, l);
    ASSERT_EQ(0.0f, r);
    prts_voice_info info;
    ASSERT_TRUE(prts_mixer_voice_info(m, music, &info));
    ASSERT_EQ(0.0f, info.fade); /* the music's one-second fade-in has not begun */
    ASSERT_TRUE(prts_mixer_playing(m, sound));

    /* the first click: the device runs, and both start from their beginning */
    prts_audio_resume(a);
    prts_audio_update(a, 1.0 / 30.0);
    ASSERT_FALSE(prts_audio_held(a));
    uint64_t mixed = prts_audio_frames_mixed(a);
    ASSERT_GT(mixed, (uint64_t)0);
    ASSERT_LT(mixed, (uint64_t)PRTS_AUDIO_RATE / 4); /* about what the device keeps queued */
    prts_audio_levels(a, &l, &r);
    ASSERT_GT(l, 0.01f);
    ASSERT_TRUE(prts_mixer_voice_info(m, music, &info));
    ASSERT_GT(info.fade, 0.0f);
    ASSERT_LT(info.fade, 0.5f);
    prts_audio_destroy(a);

    /* the null device (headless) is never held */
    a = prts_audio_create(vfs, false, NULL);
    prts_audio_pause(a);
    prts_audio_update(a, 1.0 / 30.0);
    ASSERT_FALSE(prts_audio_held(a));
    ASSERT_EQ((uint64_t)(PRTS_AUDIO_RATE / 30), prts_audio_frames_mixed(a));
    prts_audio_destroy(a);
    prts_vfs_destroy(vfs);
}

/* --- the engine: listener on the commander, sounds following entities ------------------------- */

static lua_Integer lua_int(prts_engine *e, const char *code)
{
    lua_State *L = prts_script_lua(e->script);
    char buf[512];
    snprintf(buf, sizeof(buf), "__r = %s", code);
    if (!prts_script_run_string(e->script, buf, "=test")) return -1;
    lua_getglobal(L, "__r");
    lua_Integer v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

UTEST(audio, engine_listener_and_attached_sounds)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = PRTS_TEST_TMP "/user";
    cfg.headless = true;
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    prts_mixer *m = prts_audio_mixer(e->audio);
    /* the commander stands at the start marker; turned to look along -x, +z is on the left */
    e->commander.yaw = (float)(PRTS_PI / 2);
    /* positions are taken relative to the start marker, wherever the map puts it */
    ASSERT_TRUE(prts_script_run_string(e->script, "__sx, __sy, __sz = prts.scene.marker_pos('start')", "=test"));
    prts_voice_id v = (prts_voice_id)lua_int(
        e, "prts.audio.play{ sound = 'sounds/footstep.wav', at = { __sx, __sy + 1.7, __sz + 4 }, loop = true }");
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    prts_voice_info info;
    ASSERT_TRUE(prts_mixer_voice_info(m, v, &info));
    ASSERT_GT(info.gain_left, 3.0f * info.gain_right);
    /* a sound played on an entity follows it, and stops when it is gone */
    ASSERT_TRUE(prts_script_run_string(e->script, "__noisy = prts.entity.spawn{ name = 'noisy', pos = { __sx + 3, __sy, __sz - 2 } }", "=test"));
    v = (prts_voice_id)lua_int(e, "prts.audio.play{ sound = 'sounds/torch_loop.wav', entity = __noisy, loop = true }");
    prts_entity ent = (prts_entity)lua_int(e, "__noisy");
    double sx = (double)lua_int(e, "__sx"), sz = (double)lua_int(e, "__sz");
    prts_world_set_local(e->world, ent, prts_dv3(sx + 6, 0, sz - 2), 0.0f);
    prts_world_begin_tick(e->world);
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    ASSERT_TRUE(prts_mixer_voice_info(m, v, &info));
    ASSERT_NEAR(sx + 6.0, info.pos.x, 1e-6);
    prts_world_despawn(e->world, ent);
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    ASSERT_TRUE(prts_mixer_voice_info(m, v, &info));
    ASSERT_TRUE(info.stopping);
    for (int i = 0; i < 3; i++) prts_engine_frame(e, 1.0 / 30.0);
    ASSERT_FALSE(prts_mixer_playing(m, v));
    float l, r;
    prts_audio_levels(e->audio, &l, &r);
    ASSERT_GT(l + r, 0.0f); /* the music and the footsteps play */
    prts_engine_destroy(e);
}
