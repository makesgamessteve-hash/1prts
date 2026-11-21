/* 1stPRTS engine: the audio system (mixer, device or null device, sound cache, music). MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_audio.h"
#include "prts/prts_log.h"

#define CHUNK 1024            /* frames mixed per call into the mixer */
#define AHEAD_MS 80           /* how much mixed audio the device should have queued */
#define MAX_UPDATE_MS 250     /* most audio one update produces (after a long stall) */
#define MUSIC_FADE_MIN 0.005f

typedef struct cached_sound {
    char *path;
    prts_sound *sound; /* NULL: failed to load, the beep plays instead */
} cached_sound;

struct prts_audio {
    prts_vfs *vfs;
    prts_mixer *mixer;
    prts_audio_device *device;
    prts_sound *beep;
    prts_array cache; /* cached_sound */
    prts_voice_id music;
    char *music_path;
    double null_carry; /* fractional frames of the null device */
    float level_l, level_r;
    bool held;      /* the device was not running at the last update (browser audio unlock) */
    uint64_t mixed; /* frames mixed so far */
    float scratch[CHUNK * 2];
};

prts_audio *prts_audio_create(prts_vfs *vfs, bool use_device, const char *driver)
{
    prts_audio *a = prts_calloc(1, sizeof(*a));
    a->vfs = vfs;
    a->mixer = prts_mixer_create(PRTS_AUDIO_RATE);
    a->beep = prts_sound_beep(PRTS_AUDIO_RATE);
    prts_array_init(&a->cache, sizeof(cached_sound));
    if (use_device) a->device = prts_audio_device_open(PRTS_AUDIO_RATE, driver);
    if (!a->device) prts_log_info("audio: null device (no sound output)");
    return a;
}

void prts_audio_destroy(prts_audio *a)
{
    if (!a) return;
    prts_mixer_destroy(a->mixer); /* voices reference cached sounds: stop them first */
    prts_audio_device_close(a->device);
    for (size_t i = 0; i < a->cache.len; i++) {
        cached_sound *c = prts_array_at(&a->cache, i);
        prts_free(c->path);
        prts_sound_destroy(c->sound);
    }
    prts_array_free(&a->cache);
    prts_sound_destroy(a->beep);
    prts_free(a->music_path);
    prts_free(a);
}

prts_mixer *prts_audio_mixer(prts_audio *a) { return a->mixer; }
bool prts_audio_has_device(const prts_audio *a) { return a->device != NULL; }

static prts_sound *load(prts_audio *a, const char *vpath)
{
    char *data = NULL;
    size_t size = 0;
    prts_vfs_result r = prts_vfs_read(a->vfs, vpath, &data, &size);
    if (r != PRTS_VFS_OK) {
        prts_log_error("sound '%s' could not be read (%s); playing a placeholder beep", vpath, prts_vfs_result_name(r));
        return NULL;
    }
    char err[128];
    prts_sound *s = prts_sound_decode(data, size, err, sizeof(err));
    prts_free(data);
    if (!s) prts_log_error("sound '%s' is broken (%s); playing a placeholder beep", vpath, err);
    return s;
}

const prts_sound *prts_audio_sound(prts_audio *a, const char *vpath)
{
    for (size_t i = 0; i < a->cache.len; i++) {
        cached_sound *c = prts_array_at(&a->cache, i);
        if (strcmp(c->path, vpath) == 0) return c->sound ? c->sound : a->beep;
    }
    cached_sound c = {prts_strdup(vpath), load(a, vpath)};
    prts_array_push(&a->cache, &c);
    return c.sound ? c.sound : a->beep;
}

prts_voice_id prts_audio_play(prts_audio *a, const char *vpath, const prts_voice_params *p)
{
    prts_voice_params q = *p;
    q.sound = prts_audio_sound(a, vpath);
    q.stream = NULL;
    return prts_mixer_play(a->mixer, &q);
}

static prts_stream *open_music(prts_audio *a, const char *vpath)
{
    char *data = NULL;
    size_t size = 0;
    prts_vfs_result r = prts_vfs_read(a->vfs, vpath, &data, &size);
    if (r != PRTS_VFS_OK) {
        prts_log_error("music '%s' could not be read (%s); playing a placeholder beep", vpath, prts_vfs_result_name(r));
        return NULL;
    }
    char err[128];
    prts_stream *s = prts_stream_open_ogg(data, size, true, err, sizeof(err));
    if (!s) prts_log_error("music '%s' is broken (%s); playing a placeholder beep", vpath, err);
    return s;
}

prts_voice_id prts_audio_music(prts_audio *a, const char *vpath, float fade, float volume)
{
    if (vpath && a->music_path && strcmp(vpath, a->music_path) == 0 && prts_mixer_playing(a->mixer, a->music)) {
        prts_mixer_set_volume(a->mixer, a->music, volume);
        return a->music;
    }
    float secs = fmaxf(fade, MUSIC_FADE_MIN);
    prts_mixer_fade(a->mixer, a->music, 0.0f, secs, true); /* the old track fades out while... */
    prts_free(a->music_path);
    a->music_path = NULL;
    a->music = PRTS_VOICE_NONE;
    if (!vpath) return PRTS_VOICE_NONE;
    prts_voice_params p;
    prts_voice_params_default(&p);
    p.bus = PRTS_BUS_MUSIC;
    p.volume = volume;
    p.stream = open_music(a, vpath);
    p.sound = p.stream ? NULL : a->beep;
    p.loop = p.stream != NULL;
    p.fade_in = p.stream && fade > 0.0f ? secs : 0.0f; /* ...the new one fades in */
    a->music = prts_mixer_play(a->mixer, &p);
    a->music_path = prts_strdup(vpath);
    return a->music;
}

prts_voice_id prts_audio_music_voice(const prts_audio *a)
{
    return prts_mixer_playing(a->mixer, a->music) ? a->music : PRTS_VOICE_NONE;
}

bool prts_audio_is_placeholder(const prts_audio *a, prts_voice_id v)
{
    prts_voice_info info;
    return prts_mixer_voice_info(a->mixer, v, &info) && info.sound == a->beep;
}

bool prts_audio_set_muffle(prts_audio *a, prts_voice_id v, float amount)
{
    return prts_mixer_set_muffle(a->mixer, v, amount);
}

void prts_audio_set_listener(prts_audio *a, prts_dvec3 pos, float yaw) { prts_mixer_set_listener(a->mixer, pos, yaw); }

void prts_audio_update(prts_audio *a, double real_seconds)
{
    const int rate = PRTS_AUDIO_RATE;
    int frames;
    if (a->device && !prts_audio_device_running(a->device)) {
        /* Spec: Browser audio unlock. Nothing can be heard yet, so nothing is mixed: voices
         * keep their place (a fade-in has not begun, music is at its first sample) and start
         * when the device runs, instead of playing into a device that throws them away. */
        if (!a->held) prts_log_info("audio: waiting for the device (browsers: the first click, tap or key)");
        a->held = true;
        a->level_l = a->level_r = 0.0f;
        return;
    }
    if (a->held) prts_log_info("audio: the device runs; %d held sounds start now", prts_mixer_voice_count(a->mixer));
    a->held = false;
    if (a->device) {
        /* the main thread mixes just enough to keep AHEAD_MS queued; SDL plays it on its own
         * thread. (design.md D11 planned the mixer on the audio thread behind a lock-free
         * queue; this is simpler, needs no threads on the web, and keeps the mixer single-threaded.) */
        frames = rate * AHEAD_MS / 1000 - prts_audio_device_queued(a->device);
    } else {
        double secs = real_seconds > 0.0 ? PRTS_MIN(real_seconds, MAX_UPDATE_MS / 1000.0) : 0.0;
        a->null_carry += secs * rate;
        frames = (int)a->null_carry;
        a->null_carry -= frames;
    }
    frames = PRTS_CLAMP(frames, 0, rate * MAX_UPDATE_MS / 1000);
    double sum_l = 0.0, sum_r = 0.0;
    for (int done = 0; done < frames;) {
        int n = PRTS_MIN(CHUNK, frames - done);
        prts_mixer_render(a->mixer, a->scratch, n);
        for (int i = 0; i < n; i++) {
            sum_l += (double)a->scratch[2 * i] * a->scratch[2 * i];
            sum_r += (double)a->scratch[2 * i + 1] * a->scratch[2 * i + 1];
        }
        if (a->device) prts_audio_device_write(a->device, a->scratch, n);
        done += n;
    }
    a->mixed += (uint64_t)frames;
    if (frames > 0) {
        a->level_l = (float)sqrt(sum_l / frames);
        a->level_r = (float)sqrt(sum_r / frames);
    }
}

void prts_audio_resume(prts_audio *a)
{
    if (a->device) prts_audio_device_resume(a->device);
}

void prts_audio_pause(prts_audio *a)
{
    if (a->device) prts_audio_device_pause(a->device);
}

bool prts_audio_held(const prts_audio *a) { return a->held; }
uint64_t prts_audio_frames_mixed(const prts_audio *a) { return a->mixed; }

void prts_audio_levels(const prts_audio *a, float *left, float *right)
{
    *left = a->level_l;
    *right = a->level_r;
}
