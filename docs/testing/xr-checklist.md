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
   head height, controllers, comfort options: `snap`, `vignetteOn`, `vignette`, `teleport`,
   `turn`, `seat`) from the remote console; run the checks once without it too.
2. Serve `build/web-release/bin/`: `python3 -m http.server 8000 --directory build/web-release/bin`.
3. **Quest**: USB + developer mode, `adb reverse tcp:8000 tcp:8000`, open
   `http://localhost:8000/1stprts.html` in Quest Browser. Remote console: `chrome://inspect` on
   the computer. **PC VR**: open `http://localhost:8000/1stprts.html` on the same PC.
   **Vision Pro**: serve over HTTPS (any HTTPS static host or tunnel).

## 2. Status in M1

- Done: support detection, the "Enter VR" button, session start and end, the `XRWebGLLayer` on
  the game's WebGL 2 context, the `local-floor` reference space (falling back to `local`), the
  frame handoff (SDL's loop pauses while the XR session drives frames), head pose, eye views and
  controller states reaching the engine (`prts_xr_get()`), each eye drawn into the XR
  framebuffer, controller input in the action mapper and pointer rays (13.2), head and body
  handling (13.3) and the comfort options with their VR video defaults (13.4).
- Default controls in VR: left stick walks (or aims a teleport), right stick turns, either
  trigger uses what its ray points at (the hand pulled last is the pointer hand), left stick
  press runs, A jumps, B crouches, Y opens the menu (which leaves VR: menus are drawn on the flat
  page only in M1). The options menu's VR page holds the comfort options.
- Not in M1: hand tracking, controller models (only the rays are drawn), in-headset menus and
  HUD, a teleport arc (the teleport aims with the straight ray).

## 3. Checks

| # | Covers | Steps | Expected |
|---|---|---|---|
| X1 | *No XR support* | Open the page in a desktop browser with no headset or XR runtime, and on a phone. | No "Enter VR" button anywhere; the game works normally. |
| X2 | *Entering VR* (control) | Open the page in the headset's browser (or with the PC headset connected). | An "Enter VR" button appears at the bottom right once the browser reports immersive VR support. |
| X3 | *Entering VR* (session) | Press "Enter VR" (on desktop: press Escape first to release the mouse, then click). | The browser asks for permission if needed, then an immersive session starts. `prts_test.xr()` shows `active: true, views: 2`. |
| X4 | *Entering VR* (stereo, head pose) | Look around and move your head. | The scene is drawn once per eye with correct depth (no doubled image, no swimming), following the head pose with no visible lag. Floor height matches the real floor (`local-floor`). |
| X5 | Frame handoff | In the session, watch the page on the PC monitor (PC VR) or the remote console. | The flat page stops advancing on its own: frames come from the headset (`prts_test.xr().frames` rises, `prts_test.frames()` minus that stays constant). No stutter from two loops fighting. |
| X6 | *Session ended by the headset* | Press the headset's system/menu button and choose to leave (Quest: Meta button → quit; PC: SteamVR dashboard → exit). | The game continues on the flat page with the commander at the same position; the "Enter VR" button is offered again. |
| X7 | Leaving VR from the browser | Start a session, then end it from the browser UI (or take the headset off until the browser ends the session). | Same as X6. |
| X8 | Leaving VR from the game's menu | Start a session, then press Y on the left controller (the menu button). | Same as X6, with the pause menu open on the flat page; entering again (X3) works a second time. |
| X9 | *Leaning toward a wall* | Stand near a keep wall in VR and lean your head into it physically; then walk into it in the play area. | The view does not pass through the wall's surface (it stops a hand's breadth short and slides along it); walking stops at the wall. Walking around in free space moves the view one to one. |
| X10 | *Pulling the lift lever* (controllers) | Point a controller ray at the hoist lift's call lever and press the trigger; repeat with the other hand. | Each controller shows a pointer ray (the pointer hand's turns gold on something usable); the lift is called exactly as with the `use` action. A hand pushed into a wall uses nothing behind it. |
| X11 | *Teleport movement* | In Options > VR choose Movement: Teleport. Push the left stick forward, aim at reachable floor, release the thumbstick. Then aim at a wall, and at floor behind a low wall or railing. | The aim ray turns green on reachable floor and the commander is there at once when the stick is released, without passing through walls. Red on a wall or unreachable floor, and nothing happens. |
| X12 | *Comfort defaults* | Enter XR for the first time (clear the site data first). | Vertex snapping, dithering and the crosshair are off in the headset (the flat page keeps its own settings) and the comfort vignette is on. |
| X13 | Smooth movement and turning (requirement *Comfort options*) | Move with the thumbstick; try snap and smooth turning (Options > VR > Turning); try seated mode while sitting. | All work; the edges darken while moving and turning smoothly; snap turn rotates in fixed steps (one per push) around your head; seated mode raises the view to standing eye height. |
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
