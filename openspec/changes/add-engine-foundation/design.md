# Design: Engine Foundation

## Context

The repository is empty apart from the concept and architecture documents. This change sets up
the engine structure described in `docs/architecture/README.md` and proves it end to end with
the Proving Ground map. Motivation and scope are in `proposal.md`. Behavior is in `specs/`.
This document records **how**: technology choices, the alternatives considered and the main risks.

## Goals / Non-Goals

**Goals**
- A module layout that later milestones extend without restructuring.
- Every engine module testable without a window or GPU, except the GL and SDL back ends, which
  run in CI under a virtual display with a software GL driver.
- Web, touch and WebXR running in M1, so platform risk surfaces early.

**Non-Goals**
- Performance tuning beyond the reference budgets in `docs/architecture/rendering.md`.
- Final art. Everything visual is greybox or placeholder (CC BY 4.0).
- Gameplay systems from M2 onward (units, orders, navigation, sensors).

## Decisions

### D1 · C11 with a `prts_` prefix
C11 gives `_Static_assert`, anonymous unions, `<stdatomic.h>` (where available) and is supported
by GCC, Clang, MSVC 2022 and Emscripten. *Alternatives:* C99 (no static asserts or atomics);
C17 or C23 (C23 support is uneven on MSVC). Symbols use `prts_` because C identifiers cannot
start with a digit.

### D2 · SDL3 with app callbacks
The launcher uses `SDL_MAIN_USE_CALLBACKS` (`SDL_AppInit/Iterate/Event/Quit`). It gives one
non-blocking loop that SDL drives from `requestAnimationFrame` on the web, with no
`emscripten_set_main_loop` special cases. *Alternatives:* a classic `while` loop with
Emscripten Asyncify (larger binary, slower); GLFW or sokol_app (not SDL3, which the project
requires). SDL3 comes in through CMake `FetchContent`, pinned to a release tag, with
`PRTS_USE_SYSTEM_SDL3` for distribution packages. The same source is built with `emcmake` for
the web. SDL3 supports Emscripten builds upstream.

### D3 · OpenGL: GL 3.3 core / GLES 3.0 common subset
One renderer back end targets the shared feature set (see `docs/architecture/rendering.md §1`).
Shader files have no `#version`. The engine adds a prelude (`#version 330 core` or
`#version 300 es` + precision) and compatibility macros. Desktop GL function pointers are loaded
with `SDL_GL_GetProcAddress` into a table generated from the list of functions we actually use
(about 120). On the web, Emscripten's GL bindings are linked directly. *Alternatives:* glad or
GLEW (extra generated or third-party code for no gain); GL 2.1/GLES 2 (no VAOs, instancing or
UBOs); GL 4.x (not on the web).

A **null back end** implements the same interface without drawing. Headless mode and most tests
use it.

### D4 · Lua 5.4, one VM, per-mod environments
Lua 5.4 is vendored and compiled as C (MIT). One `lua_State` hosts all scripts. Each mod's
files run with their own `_ENV` table that falls back to a shared read-only global table, so
definitions can be shared and merged by id. The VM is built with a fixed `luai_makeseed` for
reproducible table order. Budgets use `lua_sethook` with `LUA_MASKCOUNT` every 1,000
instructions, checking a high-resolution timer against the entry point's soft and hard limits.
*Alternatives:* LuaJIT (no WebAssembly support, so two runtimes); one VM per mod (makes shared
definitions and cross-mod overrides complicated); other languages such as Wren or Squirrel
(Lua was requested).

### D5 · Asset loaders
| Format | Choice | License | Alternatives considered |
|---|---|---|---|
| glTF 2.0 | cgltf | MIT | tinygltf (C++), assimp (large, C++) |
| PNG | stb_image | MIT / public domain | libpng + zlib (more dependencies) |
| OGG Vorbis | stb_vorbis | MIT / public domain | libvorbis (more dependencies), dr_libs (no Vorbis) |
| WAV | SDL3 `SDL_LoadWAV` | zlib | dr_wav |
| `.vox` | ogt_vox | MIT | own parser |

All loaders run inside the job system and return CPU-side data. Only `gfx/` uploads to the GPU.

### D6 · Own small entity store, not an ECS library
Entities are `uint32 index + uint32 generation` handles. Components are packed arrays with
sparse index maps, iterated in index order for determinism. *Alternative:* flecs (MIT, C99,
full-featured), rejected because it is a very large dependency, its internal ordering and
scheduling would have to be checked for determinism, and M1 needs fewer than 15 component
types. The store's interface is kept narrow so it could be swapped later.

### D7 · Reference frames
Each entity has `parent_frame` and a float local transform. Each frame also caches a
**double-precision world transform** that is updated once per tick, top-down. Rendering builds
camera-relative float matrices from doubles. **Frame volumes** are trigger shapes attached to
frames. When the commander's capsule center enters one, it is reparented with world transform and
velocity preserved (spec: *Changing parent without a jump*). The HQ elevator car is the
first kinematic frame, driven by a Lua script through `prts.frame` (target height, speed).

### D8 · Own character controller
A capsule controller with swept collision (sub-stepped so it cannot tunnel at sprint speed),
step-up up to 0.4 m, a 45° slope limit, ground probing and inherited platform velocity. Static
geometry is stored in a per-mesh BVH. Terrain uses height-field queries. *Alternatives:* Jolt
or Bullet (C++, large WebAssembly size, much more than a character needs); ODE (old API). Later
milestones add vehicle kinematics on the same collision queries.

### D9 · Cells and portals from glTF markers and greybox brushes
For M1, cells and portals are marked up in content: glTF nodes named `cell:<id>`,
`portal:<a>:<b>` and `volume:frame:<id>`, or the same tags on **greybox brushes**. The Proving
Ground HQ is built from **greybox brushes** defined in Lua (box, wedge, stairs, door frame,
window frame). They become meshes and collision when loaded and are textured from a small
CC BY 4.0 texture set. This avoids hand-modeling a three-storey building and gives modders a fast
blocking tool. glTF loading is still covered by props and the map table model. Deck plans
(M3) will generate the same cell and portal data.

### D10 · UI: microui behind `prts_ui`
Menus, options, the credits screen and the developer console use **microui** (MIT, about 1,100
lines of C). It produces draw commands that `gfx/` renders into the screen overlay or into
world-surface render targets. Scripts see only `prts.ui`, never microui itself. *Alternatives:*
Nuklear (larger, more styling work for the bitmap look); a custom UI (more code to cover; it
can replace microui later behind the same API).

### D11 · Audio: own mixer on SDL3 audio streams
A float mixer runs on SDL3's audio stream callback: voices, buses, distance attenuation, equal-power
panning, and a one-pole low-pass plus gain for muffling between cells. Commands come from the
main thread through a lock-free single-producer queue. The mixer core is plain C operating on
buffers, so it is unit-tested without an audio device. *Alternative:* miniaudio (MIT-0, very
capable). It overlaps with SDL3 audio and would put a second device layer next to SDL.

### D12 · Input: devices → actions
SDL3 events update device state. The action mapper turns device state into action values
(button or axis) per input scheme, using bindings from Lua defaults and user overrides.
`SDL_HINT_TOUCH_MOUSE_EVENTS` and `SDL_HINT_MOUSE_TOUCH_EVENTS` are turned off so touch and
mouse stay separate, and the scheme switches on the last-used device. The initial scheme on the web
comes from `matchMedia('(pointer: coarse)')` and `navigator.maxTouchPoints`, read by the JS glue
at start-up. The touch overlay (stick and context buttons) is drawn by `ui/`, and its layout is
defined in Lua.

### D13 · WebXR through our own Emscripten JS library
`web/prts_xr.js` (passed with `--js-library`) implements session support checks, session start
and end, the `XRWebGLLayer` setup on the engine's WebGL 2 context (`makeXRCompatible`), the
reference space (`local-floor`, falling back to `local`), and the frame driver. During a session
it calls the exported `prts_frame()` from `XRSession.requestAnimationFrame` and writes the viewer
pose, eye views, framebuffer and controller states into an engine struct. SDL3's callback loop is
paused while the session runs (`emscripten_pause_main_loop` / `resume`). *Alternative:* a
third-party Emscripten WebXR binding. That works, but our needs are small and we want full
control over the frame handoff.

### D14 · Web data and user storage
`data/` is packed with Emscripten's file packager into `1stprts.data`, mounted at `/data` and
loaded with a progress callback. The user area is `/user` on IDBFS and is synced after settings
and save writes. *Alternative:* fetching individual files on demand (good for large data, but
needs async loading in every loader; deferred until map sizes need it).

### D15 · Test tooling
| Need | Choice | Why | Alternatives |
|---|---|---|---|
| C unit tests | utest.h (Unlicense) | Single header; works on GCC, Clang, MSVC and Emscripten | Unity (more boilerplate), cmocka (Apache, separate build), Criterion (no MSVC) |
| Lua unit and scenario tests | Built into the engine (`prts.test`) | Tests need the real `prts` API, sandbox and simulation | busted (needs a separate Lua and LuaRocks, not our sandbox) |
| C coverage | GCC `--coverage` + gcovr | Standard, has a gate flag | llvm-cov (fine, but gcovr is simpler in CI) |
| Lua coverage | Own line hook + prototype walk → lcov | Must run inside the sandboxed VM and know non-executed lines | luacov (standalone Lua; does not report unexecuted lines precisely) |
| Golden images | Xvfb + Mesa llvmpipe on Linux | Deterministic software GL that is available in CI | GPU runners (not available or reproducible) |
| Browser checks | Playwright (Apache-2.0) | Device emulation with touch, scriptable page, Chromium headless with WebGL | Puppeteer (similar, fewer device profiles) |
| XR in browser checks | Our own `navigator.xr` mock injected by the test | Deterministic poses; tests the real glue code | Immersive Web Emulator extension (hard to automate headless) |

Test hooks for the browser (`window.prts_test`: commander position, cell, input scheme, XR
state) are compiled in only when the page URL has `?test=1` **and** the build has test hooks
enabled (`web-release` for CI, off in shipped builds).

### D16 · Data folder lookup
Lookup order: `--data <dir>` → `<executable dir>/data` (macOS bundle: `Contents/Resources/data`)
→ **developer builds only**: the source tree's `data/`, compiled in as a fallback so that hot
reload edits the real files. Release builds never use the source-tree fallback. The post-build
step copies `data/` next to release and package outputs.

### D17 · Coverage exclusions (initial list)
`apps/1stprts/main.c` (SDL entry point, covered by smoke runs), `engine/src/platform/web_*.c`
(Emscripten-only, not built in the Linux coverage job), `engine/src/xr/xr_webxr.c`
(Emscripten-only). Everything else in `engine/src/` counts. Third-party code is not
measured.

## Risks / Trade-offs

- **[WebXR loop handoff with SDL3 on Emscripten is fiddly]** → Build it as an early spike
  (task group 13 starts with it) and test the handoff with the mocked XR device in CI.
  *Fallback:* on the web only, drive frames from our own `requestAnimationFrame` instead of
  SDL's callback loop.
- **[95% coverage pushes against platform code]** → Keep `platform/` and `xr/` thin, put logic in
  plain C (mixer, action mapping, frame math, path rules), run GL tests under llvmpipe, and keep
  one reviewed exclusion list.
- **[Apple's OpenGL is deprecated]** → GL stays inside `gfx/` behind an interface. macOS builds in CI
  catch breakage early. A future back end is possible without touching the rest of the engine.
- **[Determinism differs between compilers and CPUs]** → M1 promises determinism only within one
  build. Hash checks run on every CI platform separately. Cross-platform determinism is
  revisited with open question Q6.
- **[Budget hook overhead in Lua]** → The count hook runs every 1,000 instructions. Measured in the
  performance benchmark. The interval can be raised in release builds.
- **[Software GL differs from real GPUs]** → Golden images guard against regressions only. Visual checks
  on real GPUs are part of the milestone review checklist.
- **[Mobile browsers vary a lot]** → Touch logic is tested with emulation in CI, plus a manual
  device checklist (one iOS Safari, one Android Chrome) per milestone.
- **[Placeholder content could be uncredited]** → The asset validator in CI fails on uncredited files
  from the first asset onward.

## Migration Plan

Greenfield. No migration. Every task group keeps CI green, so the main branch is always runnable.

## Open Questions

The concept's open questions **Q9** (touch and WebXR in M1), **Q10** (Lua 5.4 only) and **Q14**
(coverage scope with reviewed exclusions) are assumed at their defaults in this change. If any
of them is decided differently, the specs and tasks of this change are updated before work
starts on the affected task groups.
