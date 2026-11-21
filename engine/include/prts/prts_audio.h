/* 1stPRTS engine: sounds, music streams, the mixer and the audio device. MIT License. */
#ifndef PRTS_AUDIO_H
#define PRTS_AUDIO_H

#include "prts_base.h"
#include "prts_math.h"
#include "prts_vfs.h"

/* The mixer's output rate (frames per second, stereo float). */
#define PRTS_AUDIO_RATE 48000
#define PRTS_AUDIO_MAX_VOICES 64

/* --- buses ------------------------------------------------------------------------ */

typedef enum prts_audio_bus {
    PRTS_BUS_MASTER = 0,
    PRTS_BUS_EFFECTS,
    PRTS_BUS_VOICE,
    PRTS_BUS_MUSIC,
    PRTS_BUS_AMBIENCE,
    PRTS_BUS_COUNT
} prts_audio_bus;

const char *prts_audio_bus_name(prts_audio_bus bus);
bool prts_audio_bus_parse(const char *name, prts_audio_bus *out);

/* --- decoded sounds ------------------------------------------------------------------ */

/* Interleaved float PCM, 1 or 2 channels, at any rate (the mixer resamples). */
typedef struct prts_sound {
    float *samples;
    int frames;
    int channels;
    int rate;
} prts_sound;

/* A zeroed sound of the given size. */
prts_sound *prts_sound_create(int frames, int channels, int rate);
void prts_sound_destroy(prts_sound *s);
/* Decodes a whole WAV (RIFF PCM 8/16/24/32-bit or float) or OGG Vorbis file. Returns NULL and
 * writes a reason into err when the data cannot be used. */
prts_sound *prts_sound_decode(const void *data, size_t size, char *err, size_t err_size);
prts_sound *prts_sound_decode_wav(const void *data, size_t size, char *err, size_t err_size);
prts_sound *prts_sound_decode_ogg(const void *data, size_t size, char *err, size_t err_size);
/* The short placeholder beep played in place of missing or broken sounds. */
prts_sound *prts_sound_beep(int rate);

/* --- streamed music ------------------------------------------------------------------- */

/* An OGG Vorbis stream decoded a little at a time. The compressed file stays in memory;
 * decoded audio never does. */
typedef struct prts_stream prts_stream;

/* Takes ownership of `data` (allocated with prts_malloc), also on failure. */
prts_stream *prts_stream_open_ogg(void *data, size_t size, bool loop, char *err, size_t err_size);
void prts_stream_destroy(prts_stream *s);
int prts_stream_channels(const prts_stream *s); /* 1 or 2 */
int prts_stream_rate(const prts_stream *s);
/* Decodes up to `frames` interleaved frames. A looping stream wraps around seamlessly; a
 * finished one returns fewer frames (0 once it has ended). */
int prts_stream_read(prts_stream *s, float *out, int frames);

/* --- mixer (device independent) -------------------------------------------------------- */

/* Voice handle: generation in the high 24 bits, slot + 1 in the low 8 bits. 0 is no voice. */
typedef uint32_t prts_voice_id;
#define PRTS_VOICE_NONE ((prts_voice_id)0)

typedef struct prts_voice_params {
    const prts_sound *sound; /* either a decoded sound (not owned; must outlive the voice) */
    prts_stream *stream;     /* or a stream, which the voice owns from now on */
    prts_audio_bus bus;
    float volume;            /* linear gain, 1 = unchanged */
    bool loop;               /* sounds repeat; streams loop by themselves (kept for info) */
    bool positional;         /* false: plain stereo, no attenuation or panning */
    prts_dvec3 pos;          /* world position when positional */
    float ref_distance;      /* full volume up to here (meters; 0: default 1.5) */
    float max_distance;      /* silent from here on (meters; 0: default 40) */
    float fade_in;           /* seconds to fade in from silence (0: start at full volume) */
                             /* (fades follow an equal-power curve) */
    uint64_t tag;            /* free for the caller, e.g. an entity the voice follows */
} prts_voice_params;

void prts_voice_params_default(prts_voice_params *p);

typedef struct prts_voice_info {
    prts_audio_bus bus;
    float volume, muffle;
    float fade;                 /* current fade position 0..1 (equal-power curve) */
    float gain_left, gain_right; /* current total gains */
    float cutoff_hz;            /* low-pass cutoff (0 when muffling is off) */
    bool positional, loop, stopping;
    prts_dvec3 pos;
    uint64_t tag;
    const prts_sound *sound; /* NULL for streams */
} prts_voice_info;

typedef struct prts_mixer prts_mixer;

prts_mixer *prts_mixer_create(int rate);
void prts_mixer_destroy(prts_mixer *m);
int prts_mixer_rate(const prts_mixer *m);
/* Starts a voice. Returns PRTS_VOICE_NONE if every voice is busy or nothing can be played
 * (a stream passed in is destroyed then). */
prts_voice_id prts_mixer_play(prts_mixer *m, const prts_voice_params *p);
bool prts_mixer_playing(const prts_mixer *m, prts_voice_id v);
/* Fades the voice out over `fade_seconds` (at least a few milliseconds, so it never
 * clicks) and frees it. Returns false for voices that are not playing. */
bool prts_mixer_stop(prts_mixer *m, prts_voice_id v, float fade_seconds);
/* Fades the voice's gain to `target` over `seconds`; stops it there if `stop_at_end`. */
bool prts_mixer_fade(prts_mixer *m, prts_voice_id v, float target, float seconds, bool stop_at_end);
bool prts_mixer_set_volume(prts_mixer *m, prts_voice_id v, float volume);
bool prts_mixer_set_position(prts_mixer *m, prts_voice_id v, prts_dvec3 pos);
/* Muffling 0..1: quieter and duller (gain plus a one-pole low-pass), e.g. behind a door. */
bool prts_mixer_set_muffle(prts_mixer *m, prts_voice_id v, float amount);
bool prts_mixer_voice_info(const prts_mixer *m, prts_voice_id v, prts_voice_info *out);
int prts_mixer_voice_count(const prts_mixer *m);
/* Calls fn for every playing voice, in slot order. */
typedef void (*prts_mixer_each_fn)(prts_voice_id v, const prts_voice_info *info, void *user);
void prts_mixer_each(const prts_mixer *m, prts_mixer_each_fn fn, void *user);
void prts_mixer_stop_all(prts_mixer *m);

void prts_mixer_set_bus_volume(prts_mixer *m, prts_audio_bus bus, float volume);
float prts_mixer_bus_volume(const prts_mixer *m, prts_audio_bus bus);
/* The listener: the commander's head (position and yaw, 0 looking along -z). */
void prts_mixer_set_listener(prts_mixer *m, prts_dvec3 pos, float yaw);
/* Mixes `frames` stereo frames into out (overwriting it). Gain changes are ramped. */
void prts_mixer_render(prts_mixer *m, float *out, int frames);

/* Spatial helpers (exposed for tests). pan: -1 left .. 1 right. */
void prts_audio_pan_gains(float pan, float *left, float *right);
float prts_audio_attenuation(float distance, float ref_distance, float max_distance);
/* Low-pass cutoff for a muffle amount (0 for no filtering). */
float prts_audio_muffle_cutoff(float amount);

/* --- the audio device (implemented in platform/) ------------------------------------- */

typedef struct prts_audio_device prts_audio_device;

/* Opens the default playback device for stereo float frames at `rate`. driver: NULL for
 * the platform default, or an SDL audio driver name (tests use "dummy"). Returns NULL, with
 * a log message, when there is no usable device. */
prts_audio_device *prts_audio_device_open(int rate, const char *driver);
void prts_audio_device_close(prts_audio_device *d);
/* Frames written but not yet played. */
int prts_audio_device_queued(prts_audio_device *d);
bool prts_audio_device_write(prts_audio_device *d, const float *stereo, int frames);
/* Resumes a device the platform paused (browsers start audio suspended). */
void prts_audio_device_resume(prts_audio_device *d);

/* --- the audio system: mixer + device (or null device) + sound cache + music --------- */

typedef struct prts_audio prts_audio;

/* use_device false (headless) or no device available: a null device consumes mixed audio
 * at the rate prts_audio_update is told time passes. */
prts_audio *prts_audio_create(prts_vfs *vfs, bool use_device, const char *driver);
void prts_audio_destroy(prts_audio *a);
prts_mixer *prts_audio_mixer(prts_audio *a);
bool prts_audio_has_device(const prts_audio *a);
/* A cached decoded sound. Missing or broken files log an error once and give the beep. */
const prts_sound *prts_audio_sound(prts_audio *a, const char *vpath);
/* Plays a sound file (p->sound and p->stream are ignored). */
prts_voice_id prts_audio_play(prts_audio *a, const char *vpath, const prts_voice_params *p);
/* Cross-fades to a streamed music track over `fade` seconds (NULL: fade the music out).
 * Asking for the track that is already playing changes nothing. */
prts_voice_id prts_audio_music(prts_audio *a, const char *vpath, float fade, float volume);
prts_voice_id prts_audio_music_voice(const prts_audio *a);
/* True if the voice plays the beep that stands in for a missing or broken file. */
bool prts_audio_is_placeholder(const prts_audio *a, prts_voice_id v);
bool prts_audio_set_muffle(prts_audio *a, prts_voice_id v, float amount);
void prts_audio_set_listener(prts_audio *a, prts_dvec3 pos, float yaw);
/* Called once per frame: mixes ahead to keep the device fed, or advances the null device
 * by `real_seconds`. */
void prts_audio_update(prts_audio *a, double real_seconds);
/* Browser audio unlock: call from a user gesture. */
void prts_audio_resume(prts_audio *a);
/* RMS level of each output channel over the audio mixed by the last update. */
void prts_audio_levels(const prts_audio *a, float *left, float *right);

#endif
