/* 1stPRTS engine: the audio device on an SDL3 audio stream. MIT License. */
#include <SDL3/SDL.h>

#include "prts/prts_audio.h"
#include "prts/prts_log.h"

#define FRAME_BYTES ((int)(2 * sizeof(float)))

struct prts_audio_device {
    SDL_AudioStream *stream;
};

prts_audio_device *prts_audio_device_open(int rate, const char *driver)
{
    if (driver) SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, driver, SDL_HINT_OVERRIDE);
    bool ok = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (driver) SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    if (!ok) {
        prts_log_warn("audio: cannot start the audio system: %s", SDL_GetError());
        return NULL;
    }
    /* the main thread mixes ahead and queues stereo float frames; SDL converts them to the
     * device's format and rate on its own thread */
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 2, rate};
    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        prts_log_warn("audio: no playback device: %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return NULL;
    }
    SDL_ResumeAudioStreamDevice(stream);
    prts_log_info("audio: %s driver, %d Hz stereo", SDL_GetCurrentAudioDriver(), rate);
    prts_audio_device *d = prts_calloc(1, sizeof(*d));
    d->stream = stream;
    return d;
}

void prts_audio_device_close(prts_audio_device *d)
{
    if (!d) return;
    SDL_DestroyAudioStream(d->stream);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    prts_free(d);
}

int prts_audio_device_queued(prts_audio_device *d)
{
    int bytes = SDL_GetAudioStreamQueued(d->stream);
    return bytes > 0 ? bytes / FRAME_BYTES : 0;
}

bool prts_audio_device_write(prts_audio_device *d, const float *stereo, int frames)
{
    return frames > 0 && SDL_PutAudioStreamData(d->stream, stereo, frames * FRAME_BYTES);
}

void prts_audio_device_resume(prts_audio_device *d) { SDL_ResumeAudioStreamDevice(d->stream); }
