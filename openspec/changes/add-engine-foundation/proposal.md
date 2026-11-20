# Proposal: Engine Foundation ("M1 · Proving Ground")

## Why

1stPRTS has a concept (`docs/concept/`) and an architecture (`docs/architecture/`) but no code.
Every later milestone (RTS core, crewed vehicles, economy, air/sea, orbit) depends on the same
base: a portable C engine, Lua scripting with assets outside the binary, a retro renderer that
handles stereo, first-person movement through interiors and on moving platforms, and test
infrastructure that can enforce 95% coverage and automated game tests on every platform. The
riskiest parts are web, touch and WebXR support and testing at that standard. Building them first
means those risks surface before any gameplay depends on them.

## What Changes

- New **C11 engine** (`engine/`) with core, platform (SDL3), VFS, asset, gfx (OpenGL 3.3 core /
  WebGL 2), scene (reference frames, cells and portals), sim (fixed tick, entities), phys
  (character controller), audio, input, ui, script (Lua 5.4) and xr modules.
- New **launcher** `apps/1stprts` that starts the engine, mounts `data/` and runs
  `scripts/main.lua`.
- New **base game data** `data/base/` containing the boot script, input action definitions,
  shaders, placeholder art (CC BY 4.0 with `CREDITS.md`) and the **Proving Ground** map: terrain,
  a three-storey HQ building with doors, stairs, an elevator and a placeholder map table that
  shows a live in-world screen.
- **First-person commander**: walk, run, crouch, jump, collide, use doors, take stairs, ride the
  elevator. The view is always the commander's eyes.
- **Input**: keyboard and mouse, gamepad, **touch** (switches automatically on mobile web), and
  **WebXR** controllers, all mapped to Lua-defined actions.
- **WebXR**: enter and exit an immersive VR session in the browser, with stereo rendering,
  head tracking, controller pointer and comfort locomotion.
- **Testing**: C unit tests, Lua unit tests, headless automated game tests, determinism and
  replay checks, render golden tests, web end-to-end tests, and ≥ 95% coverage gates for C and
  Lua in CI.
- **Builds and CI** for Windows, Linux, macOS and Web (Emscripten), with packaged artifacts laid
  out as executable + `data/`.

## Capabilities

### New Capabilities
- `platform-targets`: building, running and packaging on Windows, Linux, macOS, web desktop, mobile web and WebXR from one codebase and one data set.
- `asset-filesystem`: loading all content from folders next to the binary, the virtual file system with mod overrides, safe paths, missing-asset handling, and per-asset credits.
- `lua-scripting`: running game logic as sandboxed Lua scripts with a versioned engine API, error containment, budgets, hot reload and a developer console.
- `simulation-core`: fixed-tick deterministic simulation, entity handles, state hashing, replays, and nested reference frames with large-world precision.
- `retro-rendering`: the low-resolution retro pipeline, meshes, sprites, voxels, shaders loaded from data, depth layers, portal visibility and in-world screens.
- `input-actions`: action-based input defined by scripts, remappable bindings for keyboard/mouse, gamepad and touch, and automatic touch switching on mobile.
- `xr-presentation`: immersive WebXR sessions with stereo views, tracked head and controllers, and comfort options.
- `commander-locomotion`: the first-person-only camera contract and the commander's movement through terrain, doors, stairs and moving platforms.
- `audio-playback`: positional sound effects, streamed music, volume buses and muffling between interior cells.
- `automated-testing`: headless mode, unit and scenario test runners, coverage measurement and gates, golden images and web end-to-end checks.

### Modified Capabilities
- None (there are no existing specs).

## Impact

- **Platforms**: all six targets get a first runnable build. Windows, Linux and macOS ship
  native executables. Web desktop and mobile web share one Emscripten build, and mobile gets
  touch controls. WebXR uses the same web build in headsets whose browsers support immersive VR.
- **Engine vs. scripts**: everything in "What Changes" that renders, simulates, loads or tests is
  engine C code. The boot flow, the action list, the Proving Ground setup, the map table screen
  content and the door/elevator logic are Lua and data.
- **Dependencies** (all permissive; details and alternatives in `design.md`): SDL 3 (zlib),
  Lua 5.4 (MIT), cgltf (MIT), stb_image and stb_vorbis (MIT/public domain), ogt_vox (MIT),
  microui (MIT), utest.h (Unlicense, tests only), Playwright (Apache-2.0, dev only), gcovr (BSD-3, CI only).
- **Repository**: new top-level `engine/`, `apps/`, `data/`, `web/`, `tests/`, `tools/`,
  `third_party/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/`.

### Out of scope
RTS gameplay (units, orders, pathfinding, the tactical picture, the real map-table interface),
enterable vehicles and ships, rotating or zero-g frames, deck-plan compiler, crew, economy,
multiplayer, native (non-web) VR, zip mod archives, the in-game editor, localization.
