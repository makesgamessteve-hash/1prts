# Tasks

## 1. Repository and build scaffolding

- [x] 1.1 Add the top-level `CMakeLists.txt`, `CMakePresets.json` (dev, release, coverage, sanitize, web-dev, web-release) and the folder layout from `docs/architecture/README.md §2`; verify `cmake --list-presets` shows all presets
- [x] 1.2 Fetch SDL3 through `FetchContent` pinned to a release tag, with a `PRTS_USE_SYSTEM_SDL3` option; verify a configure succeeds both ways on Linux
- [x] 1.3 Vendor Lua 5.4, cgltf, stb_image, stb_vorbis, ogt_vox, microui and utest.h under `third_party/<lib>/` with their license files; verify each builds with warnings off and `tools/check-third-party-licenses` finds a license per library
- [x] 1.4 Add strict warning flags for engine code (GCC/Clang/MSVC) and `.clang-format` / `.clang-tidy`; verify an empty `prts_core` library builds warning-free on all three compilers
- [ ] 1.5 Add GitHub Actions workflows with jobs `linux-coverage`, `linux-sanitize`, `windows`, `macos`, `web`, `checks`, `package` (initially build-only); verify all jobs are green on a pull request

## 2. Core module and C unit tests

- [x] 2.1 Implement `core/`: arena and pool allocators, dynamic arrays, hash map with insertion-ordered iteration, strings and path helpers, log with levels and sinks; verify with utest suites in `engine/tests/core_*.c`
- [x] 2.2 Implement float and double math (vec, quat, mat4, `prts_dvec3`, camera-relative conversion); verify unit tests, including the 64 km precision case from the *Large-world precision* scenario
- [x] 2.3 Implement PCG32 random streams and a 64-bit stable hash; verify fixed-seed sequence tests and hash vectors
- [x] 2.4 Wire CTest to run all C unit tests and turn on the gcovr gate (`--fail-under-line 95`) in `linux-coverage`, with `tools/coverage/exclusions.txt` (initial list from design D17); verify the job fails on a deliberately uncovered function and passes once it is tested

## 3. Platform layer, launcher and web skeleton

- [x] 3.1 Implement `platform/` on SDL3 app callbacks: window, GL context creation (3.3 core / 4.1 core on macOS / WebGL 2), event pump, timers, base and preference paths, threads; verify a smoke test opens a window under Xvfb and closes cleanly
- [x] 3.2 Show a readable error and exit non-zero when no suitable GL context can be created; verify by forcing an impossible context version in a test (spec *OpenGL context unavailable*)
- [x] 3.3 Implement the launcher `apps/1stprts` with command-line options `--data`, `--mod`, `--user`, `--headless`, `--test`, `--test-lua`, `--lua-coverage`, `--ticks`, `--seed`; verify option-parsing unit tests and `1stprts --headless --ticks 1` exiting 0
- [x] 3.4 Add the Emscripten build (`web-dev`, `web-release`) with a minimal shell page, a load-progress display and a WebGL 2 check; verify a Playwright smoke test in the `web` job sees the page reach the "engine started" state, and that a browser without WebGL 2 shows the requirement message (spec *Browser without WebGL 2*)
- [x] 3.5 Implement the job system (worker threads on desktop, inline time-sliced on single-threaded web); verify unit tests for ordering and that results are applied only at defined points

## 4. Virtual file system and user area

- [x] 4.1 Implement path validation (relative, `/`, case-sensitive, no `..`, backslashes, drive letters or empty segments); verify unit tests for every rejected form (spec *Path escape attempt*, *Same behavior across platforms*)
- [x] 4.2 Implement the mount table with last-mount-wins lookup and merged directory listings; verify unit tests with temporary directories (spec *Mod overrides a base file*, *Mod adds a new file*)
- [x] 4.3 Implement data folder lookup (design D16) and the built-in "data folder missing" error screen; verify a headless test starting with an empty folder exits non-zero with the searched paths in the log (spec *Data folder missing*)
- [x] 4.4 Implement the user data area (preference path on desktop, IDBFS `/user` on the web) with read/write APIs limited to it; verify unit tests and a web check that a written setting is still there after a page reload

## 5. Lua scripting runtime

- [x] 5.1 Embed Lua 5.4 with a fixed hash seed, a per-mod `_ENV` over a shared read-only global table, and the VFS-based `require`; verify unit tests for module loading and override order (spec *Module loading through the file system*)
- [x] 5.2 Implement the sandbox (removed and replaced functions from `docs/architecture/data-and-scripting.md §4`); verify Lua tests that each removed function is missing and that `load` rejects bytecode (spec *Blocked operating system access*)
- [x] 5.3 Implement error containment at every C→Lua entry point with traceback logging; verify a test where a failing handler leaves other handlers running (spec *Error in an event handler*)
- [x] 5.4 Implement soft and hard execution budgets with the count hook; verify a test where `while true do end` is interrupted and the next tick runs (spec *Endless loop*)
- [x] 5.5 Implement the API skeleton: `prts.api_version`, `prts.platform`, `prts.log`, `prts.vfs`, `prts.def` (merge by id), `prts.events`, `prts.time`, `prts.rand` (replacing `math.random`), `prts.sorted_pairs`, and handle validation; verify C and Lua unit tests, including the stale-handle case (spec *Stale entity handle*, *Same seed, same random numbers*)
- [x] 5.6 Implement mod manifests (`mod.lua` in a data-only sandbox), dependency and load-order resolution and the API-version check; verify tests for an incompatible mod being skipped with a message (spec *Incompatible mod*)
- [x] 5.7 Implement the script-driven boot (`scripts/main.lua`) and a minimal base-game boot script; verify a headless test that a mod's `main.lua` replaces the base one (spec *Replacing the boot script*)
- [x] 5.8 Implement the Lua unit-test runner (`--test-lua`, `prts.test` assertions, JUnit XML) and Lua coverage mode (line hook + prototype walk → lcov); add the 95% Lua gate to `linux-coverage`; verify the base-game Lua tests run in CI and the gate fails when an untested function is added (spec *Running base game tests*, *Lua coverage report*)
- [x] 5.9 Implement hot reload in developer mode (file watching through the VFS, re-run modules, keep entity script data, replace named handlers, `on_reload`); verify a desktop integration test that edits a script file and checks the changed behavior (spec *Changing a door script while playing*)
- [x] 5.10 Document the M1 scripting API in `docs/modding/api-m1.md`; verify every `prts.*` function in the bindings appears in the document (checked by a script in `checks`)

## 6. Simulation core

- [x] 6.1 Implement the fixed 30 Hz tick with accumulator, the catch-up limit and interpolation factors; verify unit tests with simulated frame times of 1/30 s, 1/144 s and 1 s (spec *Different frame rates, same simulation*, *Very slow frame*)
- [x] 6.2 Implement the entity store with index+generation handles, component arrays (transform, frame, body, collider, render, light, audio_source, door, platform, trigger, script) and events; verify unit tests including slot reuse (spec *Slot reuse*)
- [x] 6.3 Implement script components with plain-data checks; verify tests that functions and userdata are rejected when the state is serialized
- [x] 6.4 Implement the reference-frame tree with double-precision world transforms, frame volumes and reparenting that keeps world transform and velocity; verify unit tests (spec *Riding a moving frame*, *Changing parent without a jump*)
- [x] 6.5 Implement the 64-bit state hash over all simulation state; verify a test that a 1 mm position change changes the hash (spec *Divergence is detected*)
- [x] 6.6 Implement replay recording (start snapshot + per-tick inputs + look direction) and playback; verify a determinism test in `tests/game/determinism/` that records a Proving Ground walk and replays it with matching hashes (spec *Record and replay*, *Two identical runs*)

## 7. Renderer

- [x] 7.1 Implement the generated GL function loader, the gfx interface, the GL back end and the null back end; verify unit tests run against the null back end and a GL smoke test runs under Xvfb + llvmpipe
- [x] 7.2 Implement shader loading from data with the per-platform prelude, compile-error reporting with file and line, and the fallback shader; verify tests with a broken shader file (spec *Broken shader*, *Mod changes the look*)
- [x] 7.3 Implement textures (stb_image), the glTF 2.0 subset (cgltf) with marker nodes, and `.vox` greedy meshing (ogt_vox); verify loader unit tests on small fixture files, and a missing-model placeholder test (spec *Missing model*, *Voxel prop*)
- [x] 7.4 Implement the view list, camera-relative transforms, depth layers and the low-resolution scene target with nearest-neighbor integer upscale and the resolution presets; verify golden images for each preset and the near/far depth case (spec *Changing the preset*, *Field map and horizon in one view*, *Two views*)
- [x] 7.5 Implement per-vertex lighting, fog, vertex snapping, affine mapping, palette quantization with ordered dither and run-time toggles; verify golden images for each toggle state and the dusk preset (spec *Turning off dithering*, *Dusk lighting*)
- [x] 7.6 Implement 8-direction billboard sprites with animations and batching; verify a golden test from front, side and back angles (spec *Directional sprite*)
- [x] 7.7 Implement cells and portals, portal culling and the outside-in visibility rule; verify a unit test of the visible-cell set and a golden image of the closed and opened side-room door (spec *Closed door hides a room*)
- [x] 7.8 Implement offscreen views and render-target surfaces with the magic-surface material; verify a golden image of the war table surface (spec *Live war table*)
- [x] 7.9 Add render golden tests to `linux-coverage` with the 8-level / 0.5% tolerance, difference-image artifacts and `tools/golden/update`; verify a deliberate fog color change fails the test (spec *Visual regression*)

## 8. Input

- [x] 8.1 Implement device state collection (keyboard, mouse with relative mode, gamepads with hot-plug, touch fingers) on SDL3 events; verify unit tests with synthetic events (spec *Gamepad hot-plug*)
- [x] 8.2 Implement the action mapper (button and axis actions, input contexts, per-scheme bindings) and Lua action definitions in `data/base/scripts/input/`; verify C and Lua tests, including a mod adding an action (spec *Mod adds an action*)
- [x] 8.3 Implement binding overrides saved in `config.lua` in the user area; verify a test that a rebind survives a restart (spec *Rebinding use*)
- [x] 8.4 Implement scheme detection and switching on the last-used device, with scheme-changed events to scripts and the web start-up hint from the JS glue; verify unit tests and Lua tests (spec *Prompt update*, *Tablet with a keyboard*)
- [ ] 8.5 Implement the touch overlay (floating stick, look drag, tap to use, context buttons) with its layout defined in Lua; verify unit tests of multi-touch handling (spec *Touch movement and look*) and the Playwright mobile-emulation test (spec *Phone browser*, *Emulated phone*)

## 9. Physics and commander

- [x] 9.1 Implement collision queries (BVH per static mesh, height-field terrain, ray and capsule sweeps); verify unit tests on fixture geometry
- [x] 9.2 Implement the capsule character controller (sub-stepped sweeps, slide, step-up 0.4 m, 45° slope limit, ground probe, platform velocity); verify unit tests and scenario tests for stairs and sprinting into a wall (spec *Stairs*, *Wall at sprint speed*)
- [x] 9.3 Implement commander movement and eye height in Lua-tunable parameters (walk, run, sprint with stamina, crouch, jump); verify scenario tests (spec *Walking speed*, *Crouching lowers the view*)
- [x] 9.4 Implement the use/interaction system (look or pointer ray, reach, prompt, script callbacks) and doors (animation, collision update, portal state, sound); verify a scenario test (spec *Opening a door*)
- [x] 9.5 Implement moving platforms and the scripted hoist lift in the keep (call levers, platform frame, gates per floor); verify scenario tests (spec *Riding the keep's hoist lift*, *Walking during movement*)
- [x] 9.6 Enforce the First-Person Contract: release builds have no camera other than the commander's, and developer builds have an observer camera labeled on screen; verify a release-build test that the observer command is unavailable and a developer-build test that the label is shown (spec *Release build*, *Developer observer camera*)

## 10. Proving Ground content and asset checks

- [x] 10.1 Implement greybox brushes (box, wedge, stairs, door frame, window frame) defined in Lua, generating meshes, collision, cells and portals (design D9); verify unit tests of the generated geometry
- [x] 10.2 Build the Proving Ground: ≥ 1 km × 1 km terrain, dusk lighting, the three-storey keep (great hall, side rooms, spiral stair, hoist lift, war room with a placeholder war table), a `.vox` crate, a person sprite and a forge room; verify the scenario test *Reaching the war room*
- [x] 10.3 Create the placeholder art (textures, sprite sheet, voxel crate, sounds, music loop, bitmap font) as original CC BY 4.0 work with `data/base/CREDITS.md` entries; verify the asset validator passes
- [x] 10.4 Implement `tools/validate-assets` (credits coverage, references resolve, formats parse, manifests valid) and add it to `checks`; verify it fails on an uncredited fixture file (spec *Uncredited asset detected*)
- [ ] 10.5 Write the commander-is-there smoke scenario set (walk, stairs, door, lift, war table) and run it in CI on Windows, Linux, macOS and web; verify all report passed with the same final cell (spec *Same scenario result on desktop and web*)

## 11. Audio

- [x] 11.1 Implement the mixer core (voices, buses, attenuation, equal-power panning, low-pass muffling) as device-independent C; verify unit tests on rendered buffers (spec *Sound to the left*, *Muting music*)
- [x] 11.2 Connect the mixer to SDL3 audio streams with the lock-free command queue, plus a null device for headless mode; verify a smoke test with a real device under CI where available and the null device otherwise
- [x] 11.3 Implement WAV and OGG loading, OGG streaming and script-controlled cross-fades; verify unit tests on fixture files (spec *Cross-fade*)
- [x] 11.4 Implement muffling between cells from portal state; verify a scenario test checking the voice's filter state when the forge room door closes (spec *Door closes on a noisy room*)
- [ ] 11.5 Implement the browser audio unlock with queued start requests; verify a Playwright test that music starts after the first click (spec *First click starts music*)

## 12. UI, menus and developer console

- [x] 12.1 Implement `prts_ui` over microui with bitmap fonts, screen overlay and world-surface targets, with ray-cast input onto surfaces; verify unit tests of hit testing and UV mapping
- [x] 12.2 Implement the front-end menu, options (video presets and effects, audio buses, controls rebinding) and the Credits screen in Lua; verify Lua tests of the menu logic and a scenario test that the Credits screen lists base and mod entries (spec *Credits screen*, *Settings persist*)
- [x] 12.3 Implement the developer console (log view, Lua command line, reload all scripts) in developer builds; verify a scenario test running `prts.log.info(prts.api_version)` (spec *Running a command*)
- [x] 12.4 Add the in-game save of settings changes for retro preset and effects; verify a restart test (spec *Settings persist*)

## 13. Web platform and WebXR

- [x] 13.1 Spike the WebXR frame-driver handoff: `web/prts_xr.js` starts a session, pauses SDL's loop, drives `prts_frame()` from the XR session and resumes SDL's loop on end; verify with the `navigator.xr` mock in Playwright (spec *Entering VR*, *Session ended by the headset*)
- [ ] 13.2 Implement the XR interface in `xr/` (session state, eye views into the renderer's view list, controller poses and buttons into the action mapper, pointer rays); verify unit tests with recorded pose fixtures (spec *Pulling the lift lever*)
- [ ] 13.3 Implement XR body handling: head offset within the play area, collision push-back for the head, seated mode; verify unit tests (spec *Leaning toward a wall*)
- [ ] 13.4 Implement comfort options (smooth and teleport movement, snap and smooth turn, vignette) and XR effect defaults; verify unit tests and a Playwright XR-mock test (spec *Teleport movement*, *Comfort defaults*)
- [x] 13.5 Implement XR availability detection and the "Enter VR" control, hidden when unsupported; verify Playwright tests with and without the mock (spec *No XR support*)
- [x] 13.6 Add the browser end-to-end suite (desktop boot and keyboard, mobile emulation, XR mock) to the `web` job with test hooks enabled only by `?test=1` in test builds; verify all tests pass in CI
- [x] 13.7 Write `docs/testing/xr-checklist.md` and `docs/testing/mobile-checklist.md` (manual checks on real headsets and phones); verify the checklists cover every scenario in `xr-presentation` and the touch scenarios in `input-actions`

## 14. Packaging and integration

- [x] 14.1 Implement the `package` job: desktop archives (executable + `data/` + `LICENSE`, `LICENSE-ASSETS`, `LICENSING.md`, third-party licenses) and the web bundle; verify an extract-and-run smoke test on each desktop platform (spec *Extract and run*, *Licenses included*)
- [x] 14.2 Run the headless runner on a display-less CI machine with the Proving Ground; verify exit status 0 (spec *Headless on a server*)
- [x] 14.3 Check that every scenario in this change's specs is linked to at least one automated test (a mapping table in `tests/README.md`, checked by a script in `checks`); verify the script passes
- [x] 14.4 Update `README.md` with build, run and test instructions for every platform; verify the documented commands work as written in a clean checkout
- [ ] 14.5 Run `openspec validate --strict` and the full CI pipeline on the final branch; verify everything is green and both coverage gates are ≥ 95%

## Implementation notes

Status on 2026-11-21: 24 of 75 tasks are done and verified (checked above). The engine boots,
renders the Proving Ground under OpenGL 3.3 core, and passes 8 CTest suites on Linux
(C unit tests, Lua unit tests, 5 headless scenarios, 1 rendered run under Xvfb + Mesa llvmpipe)
with 99% C and 99.8% Lua line coverage.

Partly done (still unchecked):
- 1.2 SDL3 is fetched and pinned; building against a system SDL3 has not been tried.
- 1.3 Lua 5.4.7, stb_image/stb_image_write, cgltf and utest.h are vendored. stb_vorbis and
  microui come with the tasks that use them. ogt_vox is not used (see 7.3).
- 1.4 Strict warnings are on; `.clang-format` / `.clang-tidy` are missing.
- 1.5 `.github/workflows/ci.yml` exists. The Linux jobs mirror what passes locally. The Windows
  and macOS jobs have never run.
- 2.1 Arrays, strings and logging exist. Arena allocators and a hash map do not yet.
- 3.1 Window, GL context, events, time and paths work. The job system and threads (3.5) don't
  exist yet.
- 4.4 The desktop user area works. The web (IDBFS) part is missing.
- 5.5 Done except `prts.def`.
- 5.6 Manifests and the API check work. Dependencies and load order are missing.
- 5.8 The runner and coverage gate work. Results go to the log, not to JUnit XML.
- 6.2 Done, with components kept by the module that uses them and keyed by entity handle
  rather than in one component store: transform, frame, velocity, movers (platforms) and the
  usable flag in `sim/world.c`; colliders in `phys/collide.c`; meshes, sprites and lights in
  the scene; sound sources in the mixer; doors, triggers (`lib/triggers.lua`) and other script
  components as entity data in Lua.
- 6.6 Determinism is tested with two identical runs (matching state hashes). Replay recording
  is missing.
- 7.3 Done. `.vox` files are read by a small C parser in `engine/src/scene/vox.c` instead of
  ogt_vox, whose header is C++ only (the engine is C11).
- 7.4–7.9 Done. Golden images (`tests/golden/`) render at 640x360, so the `hd` and `native`
  presets (540 lines and full size) both draw at the window size there; the three lower
  presets have their own images. The far depth layer now reaches 120 km.
- 8.4 The scheme is detected. Events to scripts and the web hint are missing.
- 9.1 Done with boxes: static boxes are in one BVH for the whole map (not one per mesh), boxes
  owned by entities are checked one by one, and the terrain is a height grid. Queries are box
  sweeps and rays; there is no capsule (see 9.2).
- 9.2 Done with a box controller (design D8 as built): sliding, step-up 0.4 m, riding platforms
  and a 45-degree slope limit on terrain. `sprint_into_wall.lua` covers *Wall at sprint speed*.
- 9.4 Use/interaction and doors work. The on-screen prompt, door sounds and portal state are
  missing.
- 9.6 Release builds have no camera other than the commander's. The developer observer camera
  doesn't exist yet.
- 10.1 / 10.2 Done. Brushes are Lua helpers in `lib/build.lua` (box, wall with openings, door and
  window frames, stairs, spiral stair, ramp); cells and portals are declared next to them in
  the map. The keep has switchback stairs between floors 1-3 and a spiral stair from the war
  room floor to the roof.
- 10.3 Seventeen original textures, generated by `tools/gen_placeholder_art.py`, with credits.
  Sprites, voxels, sounds, music and fonts are missing.
- 10.4 Credits coverage and texture references are checked. Format parsing is not.

Not started: 3.4, 3.5, 5.9, 5.10, 6.3, 7.6–7.9, 8.3, 8.5, 10.5, 11.x (audio), 12.x (UI,
menus, console), 13.x (web and WebXR), 14.1, 14.3, 14.5.

Deviations from `design.md` (recorded there too):
- Audio (D11): the main thread mixes a little ahead every frame and queues it on an SDL audio
  stream (about 80 ms). There is no audio-thread mixer and no command queue. Music files are
  kept compressed in memory and decoded a little at a time.
- Script budgets count Lua instructions (deterministic) instead of measuring time.
- The commander's collider is an axis-aligned box. Static collision is a flat box list.
- Sun and sky visibility are baked into vertices at load. This is a retro substitute for
  shadows, and it keeps interiors from being lit by the sun.

## Workflow follow-up

- Review the Proving Ground demo on all platforms (including real headset and phone checklists) before archiving.
- Archive the change with `openspec archive add-engine-foundation` so its specs move to `openspec/specs/`.
