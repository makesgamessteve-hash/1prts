/* 1stPRTS engine: the boundary between C and the web glue in web/ (Emscripten only). MIT License. */
#ifndef PRTS_WEB_GLUE_H
#define PRTS_WEB_GLUE_H

#if defined(__EMSCRIPTEN__)

/* The user area on the web: IndexedDB-backed (IDBFS). web/prts_pre.js mounts and loads it before
 * main() runs, and IDBFS's autoPersist writes every change back to IndexedDB right after the
 * frame that made it. */
#define PRTS_WEB_USER_DIR "/user"

/* Implemented in web/prts_web.js (called from C). */
/* Shows a readable error on the page instead of a blocking alert(). */
void prts_web_show_error(const char *title, const char *message);
/* 1 if the browser plays audio: SDL's AudioContext is "running" (it is "suspended" until the
 * first user gesture). */
int prts_web_audio_running(void);

/* Implemented in engine/src/platform/web_platform.c (called from web/prts_pre.js). */
/* matchMedia('(pointer: coarse)') and navigator.maxTouchPoints say this is a touch-first
 * device: start in the touch scheme. */
void prts_web_set_touch_first(int touch_first);
/* The first click, tap or key press (and any later one while audio is still suspended). */
void prts_web_user_gesture(void);

#endif

#endif
