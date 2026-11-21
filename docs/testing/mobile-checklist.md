# Mobile browser checklist (manual)

Run this list on **one iOS Safari device and one Android Chrome device** for every milestone
before the change is archived (task 13.7, design "Mobile browsers vary a lot"). CI covers the same
behavior with Chromium's mobile emulation (`web/tests/mobile.test.js`), but only real devices show
real touch latency, Safari's quirks, the on-screen keyboard and storage eviction.

Copy the result table at the end into the milestone review and fill it in.

## 1. Getting the build onto the phone

1. Build the web bundle (see the README for emsdk):
   `cmake --preset web-release -DPRTS_WEB_TEST_HOOKS=ON && cmake --build --preset web-release`.
   With the hooks compiled in, opening the page with `?test=1` lets you read the engine's state
   from the remote console (`prts_test.scheme()`, `prts_test.commander()`); without `?test=1` the
   page behaves exactly like a shipped build. Run the checks **without** `?test=1` first.
2. Serve `build/web-release/bin/` from your computer, for example
   `python3 -m http.server 8000 --directory build/web-release/bin`.
3. Open the page on the phone:
   - **Android Chrome**: connect USB, run `adb reverse tcp:8000 tcp:8000`, open
     `http://localhost:8000/1stprts.html`. Remote console: `chrome://inspect` on the computer.
   - **iOS Safari**: same Wi-Fi network, open `http://<computer-ip>:8000/1stprts.html`. Remote
     console: Safari on a Mac, *Develop* menu → the phone (turn on *Web Inspector* in the
     phone's Safari settings first).
4. Note the device, OS version and browser version in the result table.

## 2. Checks

Each line names the requirement or scenario it covers (`openspec/changes/archive/2026-11-21-add-engine-foundation/specs/`).

| # | Covers | Steps | Expected |
|---|---|---|---|
| M1 | platform-targets: *Web bundle runs in a browser* | Open the page on mobile data or Wi-Fi with an empty cache. | The loading panel shows "Downloading game data… N%" counting up, then the Proving Ground appears. No blank or white screen at any point. |
| M2 | input-actions: *Phone browser* | Open the page; touch nothing in the settings. | The game starts in the **touch** scheme: the touch controls (floating stick area, context buttons when usable) are visible without any setting being changed. With `?test=1`: `prts_test.scheme()` is `"touch"` before the first touch. |
| M3 | input-actions: *Touch movement and look* | Drag on the left half of the screen and, at the same time, drag on the right half. | The commander walks in the dragged direction (floating stick appears where the left finger landed) while the view turns with the right finger. Neither drag scrolls, zooms or selects the page. |
| M4 | input-actions: *Automatic touch controls* (tap to use) | Walk up to the keep's door, tap it on the right half. | The door opens, the same as the `use` action on desktop. |
| M5 | input-actions: *Automatic touch controls* (context buttons) | Walk away from any usable object, then back to the door. | Context buttons appear only while something can be used, and disappear again. |
| M6 | input-actions: *Tablet with a keyboard* | On a tablet (or phone) with a Bluetooth/attached keyboard: while touch controls are shown, press **W**. | The touch overlay hides and keyboard (and mouse, if attached) control the commander. `prts_test.scheme()` is `"kbm"`. |
| M7 | input-actions: *Automatic touch controls* (back to touch) | After M6, touch the screen once. | The touch controls come back on the next touch. `prts_test.scheme()` is `"touch"`. |
| M8 | automated-testing: *Emulated phone* (real-device counterpart) | Drag on the right half only. | The view turns; the commander does not move. |
| M9 | audio-playback: *First click starts music* | Open the page with the sound on; tap once. | Music and sounds start after the first tap, and sounds requested before the tap are not lost. (iOS: the hardware silent switch mutes Web Audio; check with it off.) |
| M10 | asset-filesystem: user data persists between visits | Change a setting (or, with `?test=1`, run `prts_test.userWrite('settings/check.txt', 'kept')`), close the tab, open the page again. | The setting is still there (`prts_test.userRead('settings/check.txt')` is `"kept"`). |
| M11 | platform-targets: *Browser without WebGL 2* | Only where possible (for example an old iOS version, or Chrome with `chrome://flags` WebGL 2 disabled). | A readable "WebGL 2 is required" message instead of a blank screen. |
| M12 | Orientation and size | Rotate the phone during play; open and close the browser's toolbars by scrolling attempts. | The view fills the screen in both orientations, nothing is cut off by the notch (safe areas), and the aspect ratio is right after each rotation. |
| M13 | Background and return | Switch to another app for 30 s and come back; lock and unlock the phone. | The game continues where it was; no reload, no stuck input (no "ghost" finger still moving the commander). |
| M14 | xr-presentation: *No XR support* (phone) | On a phone browser without immersive VR. | No "Enter VR" button is shown, and the game works normally. |
| M15 | Performance | Walk through the keep (great hall, stairs, war room). | Movement stays smooth; note the rough frame rate (remote console, *Performance* panel) in the table. |

## 3. Known platform notes

- **iOS Safari** may evict IndexedDB data for sites that were not used for 7 days (and in
  Private Browsing nothing persists). M10 checks the normal case only.
- **iOS Safari** has no pointer lock; mouse look on an iPad with a trackpad uses unlocked mouse
  motion. Record what happens in the notes column.
- Pinch-zoom and double-tap-zoom must not trigger on the game canvas (the page sets
  `touch-action: none` and a non-scalable viewport).

## 4. Result table

| # | iOS Safari (device, iOS x.y) | Android Chrome (device, Android x, Chrome y) | Notes |
|---|---|---|---|
| M1 | | | |
| M2 | | | |
| M3 | | | |
| M4 | | | |
| M5 | | | |
| M6 | | | |
| M7 | | | |
| M8 | | | |
| M9 | | | |
| M10 | | | |
| M11 | | | |
| M12 | | | |
| M13 | | | |
| M14 | | | |
| M15 | | | |
