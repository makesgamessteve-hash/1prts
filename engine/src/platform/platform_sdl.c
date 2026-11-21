/* 1stPRTS engine: window, GL context, events and time on SDL3. MIT License. */
#include <SDL3/SDL.h>

#include "prts/prts_log.h"
#include "prts/prts_platform.h"

struct prts_platform {
    SDL_Window *window;
    SDL_GLContext gl;
    bool gles;
};

prts_platform *prts_platform_create(const prts_platform_config *cfg)
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        prts_log_error("cannot start the video system: %s", SDL_GetError());
        return NULL;
    }
    bool gles = false;
#if defined(__EMSCRIPTEN__)
    gles = true;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cfg->gl_major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cfg->gl_minor);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (cfg->hidden) flags |= SDL_WINDOW_HIDDEN;
    SDL_Window *win = SDL_CreateWindow(cfg->title, cfg->width, cfg->height, flags);
    SDL_GLContext ctx = win ? SDL_GL_CreateContext(win) : NULL;
    if (!ctx) {
        prts_log_error("1stPRTS needs OpenGL %d.%d (core profile) or WebGL 2, which this system could not provide: %s",
                       cfg->gl_major, cfg->gl_minor, SDL_GetError());
        if (win) SDL_DestroyWindow(win);
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
        return NULL;
    }
    SDL_GL_SetSwapInterval(1);
    prts_platform *p = prts_calloc(1, sizeof(*p));
    p->window = win;
    p->gl = ctx;
    p->gles = gles;
    return p;
}

void prts_platform_destroy(prts_platform *p)
{
    if (!p) return;
    SDL_GL_DestroyContext(p->gl);
    SDL_DestroyWindow(p->window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    prts_free(p);
}

bool prts_platform_gles(const prts_platform *p) { return p->gles; }

void *prts_platform_gl_getproc(const char *name)
{
    SDL_FunctionPointer fp = SDL_GL_GetProcAddress(name);
    void *out;
    SDL_memcpy(&out, &fp, sizeof(out));
    return out;
}

void prts_platform_swap(prts_platform *p) { SDL_GL_SwapWindow(p->window); }

void prts_platform_drawable_size(const prts_platform *p, int *w, int *h)
{
    SDL_GetWindowSizeInPixels(p->window, w, h);
}

void prts_platform_set_relative_mouse(prts_platform *p, bool on) { SDL_SetWindowRelativeMouseMode(p->window, on); }

void prts_platform_begin_frame(prts_input_state *s)
{
    s->mouse_dx = s->mouse_dy = s->wheel = 0.0f;
}

static void open_gamepads(prts_input_state *s)
{
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    s->pad_connected = false;
    for (int i = 0; i < count; i++) {
        if (SDL_OpenGamepad(ids[i])) s->pad_connected = true;
    }
    SDL_free(ids);
}

bool prts_platform_apply_event(prts_platform *p, const void *sdl_event, prts_input_state *s)
{
    PRTS_UNUSED(p);
    const SDL_Event *e = sdl_event;
    switch (e->type) {
    case SDL_EVENT_QUIT: return true;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        if ((int)e->key.scancode < PRTS_INPUT_MAX_KEYS) s->keys[e->key.scancode] = e->type == SDL_EVENT_KEY_DOWN;
        s->last_device = PRTS_SCHEME_KBM;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        s->mouse_dx += e->motion.xrel;
        s->mouse_dy += e->motion.yrel;
        if (e->motion.which != SDL_TOUCH_MOUSEID) s->last_device = PRTS_SCHEME_KBM;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e->button.button < PRTS_INPUT_MAX_BUTTONS) s->mouse[e->button.button] = e->button.down;
        s->last_device = PRTS_SCHEME_KBM;
        break;
    case SDL_EVENT_MOUSE_WHEEL: s->wheel += e->wheel.y; break;
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED: open_gamepads(s); break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (e->gaxis.axis < PRTS_INPUT_MAX_PAD_AXES) s->pad_axes[e->gaxis.axis] = (float)e->gaxis.value / 32767.0f;
        if (e->gaxis.value > 8000 || e->gaxis.value < -8000) s->last_device = PRTS_SCHEME_GAMEPAD;
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (e->gbutton.button < PRTS_INPUT_MAX_PAD_BUTTONS) s->pad_buttons[e->gbutton.button] = e->gbutton.down;
        s->last_device = PRTS_SCHEME_GAMEPAD;
        break;
    case SDL_EVENT_FINGER_DOWN:
        s->fingers++;
        s->last_device = PRTS_SCHEME_TOUCH;
        break;
    case SDL_EVENT_FINGER_UP:
        if (s->fingers > 0) s->fingers--;
        break;
    default: break;
    }
    return false;
}

bool prts_platform_poll(prts_platform *p, prts_input_state *s)
{
    SDL_Event e;
    bool quit = false;
    while (SDL_PollEvent(&e)) quit = prts_platform_apply_event(p, &e, s) || quit;
    return quit;
}

int prts_platform_key_from_name(const char *name)
{
    SDL_Scancode sc = SDL_GetScancodeFromName(name);
    return sc == SDL_SCANCODE_UNKNOWN ? 0 : (int)sc;
}

uint64_t prts_platform_ticks_ns(void) { return SDL_GetTicksNS(); }

void prts_platform_error_box(const char *title, const char *message)
{
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, NULL)) {
        prts_log_error("%s: %s", title, message);
    }
}
