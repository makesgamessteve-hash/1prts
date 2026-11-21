/* 1stPRTS engine: XR session state (WebXR on the web), filled in by the platform glue. MIT License. */
#ifndef PRTS_XR_H
#define PRTS_XR_H

#include "prts_base.h"

#define PRTS_XR_MAX_VIEWS 2
#define PRTS_XR_MAX_CONTROLLERS 2
#define PRTS_XR_MAX_BUTTONS 8
#define PRTS_XR_MAX_AXES 4

/* A tracked pose in the session's reference space (local-floor, or local as a fallback):
 * metres, y up, orientation as a unit quaternion (x, y, z, w). */
typedef struct prts_xr_pose {
    float position[3];
    float orientation[4];
} prts_xr_pose;

/* One eye. Matrices are column-major, as WebXR delivers them. */
typedef struct prts_xr_view {
    float view[16];       /* reference space -> eye (the inverse of the eye's transform) */
    float projection[16];
    int viewport[4];      /* x, y, width, height inside the XR framebuffer */
} prts_xr_view;

typedef enum prts_xr_hand {
    PRTS_XR_HAND_NONE = 0,
    PRTS_XR_HAND_LEFT,
    PRTS_XR_HAND_RIGHT
} prts_xr_hand;

typedef struct prts_xr_controller {
    bool connected;
    prts_xr_hand hand;
    bool has_grip, has_aim;
    prts_xr_pose grip; /* where the controller is held */
    prts_xr_pose aim;  /* the pointer ray: origin and -z direction */
    int button_count, axis_count;
    float buttons[PRTS_XR_MAX_BUTTONS]; /* analog values 0..1 (xr-standard gamepad order) */
    bool pressed[PRTS_XR_MAX_BUTTONS];
    float axes[PRTS_XR_MAX_AXES];       /* -1..1: touchpad x, y, thumbstick x, y */
} prts_xr_controller;

typedef struct prts_xr {
    bool supported;   /* the browser offers immersive VR sessions */
    bool active;      /* an immersive session is running */
    uint64_t sessions; /* sessions started so far */
    uint64_t frames;   /* frames delivered by XR sessions so far */
    unsigned framebuffer; /* GL name of the XR layer's framebuffer (0: none) */
    int fb_width, fb_height;
    bool has_pose;      /* false while tracking is lost */
    prts_xr_pose head;  /* the viewer (between the eyes) */
    int view_count;
    prts_xr_view views[PRTS_XR_MAX_VIEWS];
    prts_xr_controller controllers[PRTS_XR_MAX_CONTROLLERS];
} prts_xr;

/* The XR state of this process. A browser page has at most one immersive session, so there
 * is exactly one; the web glue (engine/src/xr/xr_webxr.c) writes it and the engine reads it. */
prts_xr *prts_xr_get(void);
void prts_xr_reset(prts_xr *xr);
/* Starts looking for immersive VR support once the window and GL context exist. On the web this
 * asks the browser (asynchronously; web/prts_xr.js then shows "Enter VR" if it is supported).
 * Other platforms have no XR yet, so supported stays false. */
void prts_xr_detect(prts_xr *xr);
void prts_xr_set_supported(prts_xr *xr, bool supported);
/* A session started; it renders into the given framebuffer of the given size. */
void prts_xr_begin(prts_xr *xr, unsigned framebuffer, int fb_width, int fb_height);
/* The session ended (from the game, the browser or the headset): views and controllers are
 * cleared, the counters are kept. */
void prts_xr_end(prts_xr *xr);

/* Packed frame data, as the web glue (web/prts_xr.js) writes it into a float array once per
 * XR frame. All offsets are in floats. Flags are 0 or 1; counts are clamped when read. */
enum {
    PRTS_XR_F_VIEW_COUNT = 0,
    PRTS_XR_F_HAS_POSE = 1,
    PRTS_XR_F_HEAD = 2,          /* position x y z, orientation x y z w */
    PRTS_XR_F_VIEWS = 9,         /* PRTS_XR_MAX_VIEWS blocks of PRTS_XR_F_VIEW_SIZE */
    PRTS_XR_F_VIEW_SIZE = 36,    /* view[16], projection[16], viewport[4] */
    PRTS_XR_F_CONTROLLERS = PRTS_XR_F_VIEWS + PRTS_XR_MAX_VIEWS * PRTS_XR_F_VIEW_SIZE,
    /* per controller: connected, hand, has_grip, grip[7], has_aim, aim[7], button_count,
     * pressed bit mask, buttons[8], axis_count, axes[4] */
    PRTS_XR_F_CONTROLLER_SIZE = 33,
    PRTS_XR_FRAME_FLOATS = PRTS_XR_F_CONTROLLERS + PRTS_XR_MAX_CONTROLLERS * PRTS_XR_F_CONTROLLER_SIZE
};

/* Reads one packed frame into xr and counts the frame. Returns false (and changes nothing)
 * when no session is active or the data is too short. */
bool prts_xr_apply_frame(prts_xr *xr, const float *data, size_t count);

#endif
