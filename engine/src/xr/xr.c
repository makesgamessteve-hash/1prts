/* 1stPRTS engine: XR session state and the packed per-frame data from the web glue. MIT License. */
#include "prts/prts_xr.h"

#include <string.h>

static prts_xr g_xr;

#if defined(__EMSCRIPTEN__)
void prts_xr_web_init(void); /* web/prts_xr.js */
#endif

prts_xr *prts_xr_get(void) { return &g_xr; }

void prts_xr_reset(prts_xr *xr) { memset(xr, 0, sizeof(*xr)); }

void prts_xr_detect(prts_xr *xr)
{
    xr->supported = false;
#if defined(__EMSCRIPTEN__)
    prts_xr_web_init(); /* answers later through prts_xr_web_set_supported() */
#endif
}

void prts_xr_set_supported(prts_xr *xr, bool supported) { xr->supported = supported; }

void prts_xr_begin(prts_xr *xr, unsigned framebuffer, int fb_width, int fb_height)
{
    prts_xr_end(xr);
    xr->active = true;
    xr->sessions++;
    xr->framebuffer = framebuffer;
    xr->fb_width = fb_width;
    xr->fb_height = fb_height;
}

void prts_xr_end(prts_xr *xr)
{
    xr->active = false;
    xr->framebuffer = 0;
    xr->fb_width = xr->fb_height = 0;
    xr->has_pose = false;
    xr->view_count = 0;
    memset(&xr->head, 0, sizeof(xr->head));
    memset(xr->views, 0, sizeof(xr->views));
    memset(xr->controllers, 0, sizeof(xr->controllers));
}

static int clamp_count(float v, int max)
{
    if (!(v > 0.0f)) return 0; /* also rejects NaN */
    return v >= (float)max ? max : (int)v;
}

static void read_pose(prts_xr_pose *p, const float *d)
{
    memcpy(p->position, d, sizeof(p->position));
    memcpy(p->orientation, d + 3, sizeof(p->orientation));
}

static void read_controller(prts_xr_controller *c, const float *d)
{
    memset(c, 0, sizeof(*c));
    c->connected = d[0] != 0.0f;
    if (!c->connected) return;
    int hand = clamp_count(d[1], PRTS_XR_HAND_RIGHT);
    c->hand = (prts_xr_hand)hand;
    c->has_grip = d[2] != 0.0f;
    if (c->has_grip) read_pose(&c->grip, d + 3);
    c->has_aim = d[10] != 0.0f;
    if (c->has_aim) read_pose(&c->aim, d + 11);
    c->button_count = clamp_count(d[18], PRTS_XR_MAX_BUTTONS);
    unsigned mask = (unsigned)clamp_count(d[19], 1 << PRTS_XR_MAX_BUTTONS);
    for (int i = 0; i < c->button_count; i++) {
        c->buttons[i] = d[20 + i];
        c->pressed[i] = (mask >> i) & 1u;
    }
    c->axis_count = clamp_count(d[28], PRTS_XR_MAX_AXES);
    for (int i = 0; i < c->axis_count; i++) c->axes[i] = d[29 + i];
}

bool prts_xr_apply_frame(prts_xr *xr, const float *data, size_t count)
{
    if (!xr->active || !data || count < (size_t)PRTS_XR_FRAME_FLOATS) return false;
    xr->frames++;
    xr->has_pose = data[PRTS_XR_F_HAS_POSE] != 0.0f;
    xr->view_count = xr->has_pose ? clamp_count(data[PRTS_XR_F_VIEW_COUNT], PRTS_XR_MAX_VIEWS) : 0;
    if (xr->has_pose) read_pose(&xr->head, data + PRTS_XR_F_HEAD);
    for (int v = 0; v < PRTS_XR_MAX_VIEWS; v++) {
        prts_xr_view *view = &xr->views[v];
        if (v >= xr->view_count) {
            memset(view, 0, sizeof(*view));
            continue;
        }
        const float *d = data + PRTS_XR_F_VIEWS + v * PRTS_XR_F_VIEW_SIZE;
        memcpy(view->view, d, sizeof(view->view));
        memcpy(view->projection, d + 16, sizeof(view->projection));
        for (int i = 0; i < 4; i++) view->viewport[i] = (int)d[32 + i];
    }
    for (int c = 0; c < PRTS_XR_MAX_CONTROLLERS; c++) {
        read_controller(&xr->controllers[c], data + PRTS_XR_F_CONTROLLERS + c * PRTS_XR_F_CONTROLLER_SIZE);
    }
    return true;
}
