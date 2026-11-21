# WebXR headset checklist (manual)

Run this list on real headsets for every milestone before the change is archived (task 13.7).
CI enters and leaves a session with a mocked `navigator.xr` (`web/tests/xr.test.js`), which checks
the glue and the frame-driver handoff but not tracking, optics, comfort or controllers. Every
scenario of `openspec/changes/add-engine-foundation/specs/xr-presentation/spec.md` is covered
below.

Headsets, at least:
- **Meta Quest 3 or 3S**, Meta Quest Browser (standalone).
- **A PC VR headset** (for example Valve Index or Quest via Link) with Chrome or Edge on Windows,
  OpenXR runtime set (SteamVR or Meta).
- Optional: **Apple Vision Pro**, Safari (WebXR immersive-vr).

Copy the result table at the end into the milestone review and fill it in.

## 1. Getting the build into the headset

WebXR needs a secure context: `https://`, or `http://localhost`.

1. Build: `cmake --preset web-release -DPRTS_WEB_TEST_HOOKS=ON && cmake --build --preset web-release`.
   Opening the page with `?test=1` lets you read `prts_test.xr()` (session state, view count,
   head height, controllers) from the remote console; run the checks once without it too.
2. Serve `build/web-release/bin/`: `python3 -m http.server 8000 --directory build/web-release/bin`.
3. **Quest**: USB + developer mode, `adb reverse tcp:8000 tcp:8000`, open
   `http://localhost:8000/1stprts.html` in Quest Browser. Remote console: `chrome://inspect` on
   the computer. **PC VR**: open `http://localhost:8000/1stprts.html` on the same PC.
   **Vision Pro**: serve over HTTPS (any HTTPS static host or tunnel).

## 2. Status in M1

- Done: support detection, the "Enter VR" button, session start and end, the `XRWebGLLayer` on
  the game's WebGL 2 context, the `local-floor` reference space (falling back to `local`), the
  frame handoff (SDL's loop pauses while the XR session drives frames), and head pose, eye views
  and controller states reaching the engine (`prts_xr_get()`).
- **Pending**: drawing each eye into the XR framebuffer (multi-view renderer; until then the
  headset shows a cleared layer and the flat view keeps drawing on the page), controller input
  and pointer rays in the action mapper (13.2), head/body handling (13.3) and comfort options
  (13.4). Mark those checks "pending" until the matching task lands; do not skip them.

## 3. Checks

| # | Covers | Steps | Expected |
|---|---|---|---|
| X1 | *No XR support* | Open the page in a desktop browser with no headset or XR runtime, and on a phone. | No "Enter VR" button anywhere; the game works normally. |
| X2 | *Entering VR* (control) | Open the page in the headset's browser (or with the PC headset connected). | An "Enter VR" button appears at the bottom right once the browser reports immersive VR support. |
| X3 | *Entering VR* (session) | Press "Enter VR" (on desktop: press Escape first to release the mouse, then click). | The browser asks for permission if needed, then an immersive session starts. `prts_test.xr()` shows `active: true, views: 2`. |
| X4 | *Entering VR* (stereo, head pose) | Look around and move your head. | The scene is drawn once per eye with correct depth (no doubled image, no swimming), following the head pose with no visible lag. Floor height matches the real floor (`local-floor`). **Pending** the multi-view renderer. |
| X5 | Frame handoff | In the session, watch the page on the PC monitor (PC VR) or the remote console. | The flat page stops advancing on its own: frames come from the headset (`prts_test.xr().frames` rises, `prts_test.frames()` minus that stays constant). No stutter from two loops fighting. |
| X6 | *Session ended by the headset* | Press the headset's system/menu button and choose to leave (Quest: Meta button → quit; PC: SteamVR dashboard → exit). | The game continues on the flat page with the commander at the same position; the "Enter VR" button is offered again. |
| X7 | Leaving VR from the browser | Start a session, then end it from the browser UI (or take the headset off until the browser ends the session). | Same as X6. |
| X8 | Leaving VR from the game's menu | Start a session, then leave it through the game's menu (when the menu exists; until then run `prts_test.exitXR()` in the console). | Same as X6, and entering again (X3) works a second time. |
| X9 | *Leaning toward a wall* | Stand near a keep wall in VR and lean your head into it physically. | The view does not pass through the wall's surface. **Pending** 13.3. |
| X10 | *Pulling the lift lever* (controllers) | Point a controller ray at the hoist lift's call lever and press the trigger. | Each controller shows a pointer ray; the lift is called exactly as with the `use` action. **Pending** 13.2. |
| X11 | *Teleport movement* | Choose teleport movement, aim at reachable floor, release the thumbstick. | The commander moves there without passing through walls. **Pending** 13.4. |
| X12 | *Comfort defaults* | Enter XR for the first time (clear the site data first). | Vertex snapping is off and the comfort vignette is on. **Pending** 13.4. |
| X13 | Smooth movement and turning (requirement *Comfort options*) | Move with the thumbstick; try snap and smooth turning; try seated mode. | All work; snap turn rotates in fixed steps; seated mode raises the view to standing eye height. **Pending** 13.4. |
| X14 | Simulation keeps running | Enter VR while the lift is moving, leave again. | The lift has kept moving (same simulation, no restart); no jump in time when the flat page takes over again. |
| X15 | Re-entry and stability | Enter and leave VR five times in a row. | No errors in the console, no growing memory, no stuck "Enter VR" state. |

## 4. Result table

| # | Quest 3/3S (Quest Browser x.y) | PC VR (headset, Chrome/Edge x, runtime) | Vision Pro (optional) | Notes |
|---|---|---|---|---|
| X1 | | | | |
| X2 | | | | |
| X3 | | | | |
| X4 | | | | |
| X5 | | | | |
| X6 | | | | |
| X7 | | | | |
| X8 | | | | |
| X9 | | | | |
| X10 | | | | |
| X11 | | | | |
| X12 | | | | |
| X13 | | | | |
| X14 | | | | |
| X15 | | | | |
