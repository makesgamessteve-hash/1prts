# Quality & Build

## 1. Test layers

| Layer | What it checks | Where | Runs on |
|---|---|---|---|
| **C unit tests** | Each engine module on its own (core, vfs, asset loaders, sim, frames, phys, input mapping, script bindings, gfx with the null and GL backends…) | `engine/tests/` | Every CI platform |
| **Lua unit tests** | Base-game scripts (rules, definitions, UI logic, input defaults) | `data/base/scripts/tests/` | Every CI platform, inside the engine (`--test-lua`) |
| **Integration tests** | Engine and base scripts together, headless: boot, load a map, spawn entities | `tests/game/integration/` | Every CI platform |
| **Automated game tests** | Scripted play sessions in headless mode: simulated input, assertions on world state | `tests/game/scenarios/` | Every CI platform |
| **Determinism tests** | Run a scenario twice, and record then replay: identical state-hash sequences | `tests/game/determinism/` | Every CI platform |
| **Render golden tests** | Fixed camera views rendered with the real GL backend, compared with reference images | `tests/game/golden/` | Linux (Mesa llvmpipe) |
| **Web end-to-end tests** | The real web build in a browser: boot, controls, touch switching, XR session (mocked device) | `web/tests/` | Linux + Playwright (Chromium) |
| **Performance benchmarks** | Frame and tick times with N entities, headless and rendered | `tests/perf/` | Linux; reported, not a gate (for now) |

## 2. Coverage gate: at least 95%

- **C**: GCC `--coverage` build (preset `coverage`), report and gate with **gcovr**:
  `--fail-under-line 95` over `engine/src/**`. The GL backend is covered by running the render
  tests under **Xvfb + Mesa llvmpipe** in the coverage job, so rendering code counts too.
- **Lua**: the engine has a **coverage mode** (`--lua-coverage <out.info>`). It records executed
  lines with a line hook and finds executable lines by walking the function prototypes of every
  loaded chunk. The output is in lcov format. The gate is **95% of lines** over
  `data/base/scripts/**`, not counting `scripts/tests/**`.
- **Branch coverage** is reported for both, but it is not a gate.
- **Exclusions** are listed in `tools/coverage/exclusions.txt`. Each line names one file or
  function and gives a reason, for example the platform entry point `apps/1stprts/main.c`, or
  Emscripten-only glue that cannot run in the Linux coverage job. Adding an exclusion needs review.
  Inline exclusion markers are not allowed.
- Platform-specific code that is not compiled on Linux (Windows, macOS and Emscripten-only
  `#if` branches) does not show up in the Linux coverage run. It is kept as **thin as possible**
  and covered by the build-and-test jobs on those platforms.

## 3. Automated game tests

Game tests are Lua scripts run by the engine in **headless mode** (`--headless`, null renderer,
null audio, fixed seed, isolated user directory). They drive the game through the **same action
layer** that real input uses, so they test what a player can actually do.

```lua
-- tests/game/scenarios/keep_lift.lua
return prts.test.scenario {
  name = "commander rides the keep's hoist lift to the war room",
  map  = "maps/proving_ground",
  seed = 1234,
  run  = function(t)
    local cmdr = t.commander()
    t.place(cmdr, "markers/keep_hall_lift")
    t.input.press("use")                                  -- pull the call lever
    t.wait_until(function() return t.door_open("keep.lift.gate_ground") end, 15)
    t.input.hold("move_forward", 1.5)                     -- simulated seconds
    t.wait_until(function() return t.parent_frame(cmdr) == "keep.lift.platform" end, 5)
    t.input.press("use")                                  -- lever: top floor
    t.wait_until(function() return t.door_open("keep.lift.gate_floor3") end, 30)
    t.input.hold("move_forward", 2.0)
    t.expect(t.in_cell(cmdr, "keep.floor3.war_room")).is_true()
    t.expect(t.parent_frame(cmdr)).equals("world")
  end,
}
```

- **Running**: `1stprts --headless --test tests/game/scenarios/` runs every scenario. CTest runs
  each scenario as its own test, with JUnit XML output for CI.
- **Assertions** are about game state (cells, frames, positions with tolerance, component
  values, events fired), not pixels.
- **Fast**: headless ticks run as fast as possible. A 60-second scenario takes well under a second.
- **Recording**: in developer builds, any manual play session can be recorded as a replay
  and turned into a regression test.

## 4. Render golden tests

- Fixed scenes and camera positions, rendered at the internal resolution through the real GL
  backend with all retro effects in a fixed state.
- Compared with `tests/game/golden/*.png`. A test passes if no channel differs by more than 8
  and at most 0.5% of pixels differ. Diff images are kept as CI artifacts.
- Reference images are only updated by an explicit command (`tools/golden/update`), and the
  change shows up in review.

## 5. Web end-to-end tests (Playwright)

- **Desktop Chromium**: page loads, the data file downloads, the game reaches the menu,
  starts Proving Ground, and keyboard input moves the commander (read through a test hook
  enabled by a URL parameter).
- **Mobile emulation** (touch device profile): touch controls appear on their own, a drag
  on the left half moves, a drag on the right half turns the view, and using a mouse switches
  back.
- **WebXR (mocked)**: a mock `navigator.xr` device is injected, the "Enter VR" control starts
  an immersive session, the engine reports XR active with two views, and the session ends
  cleanly. Real-headset checks are a manual checklist for each milestone
  (`docs/testing/xr-checklist.md`, written in M1).

## 6. Static checks

- Warnings as errors: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` (GCC/Clang), `/W4 /WX`
  (MSVC), on engine code. Third-party code is built with warnings off.
- **ASan + UBSan** job (Clang, Linux) runs all C, Lua and game tests.
- **clang-tidy** and **clang-format** checks, with the config in the repo.
- **Asset checks** (`tools/validate-assets`): every file under `data/` is covered by a
  `CREDITS.md` entry, every reference resolves, formats parse, and manifests are valid.
- **OpenSpec**: `openspec validate --strict` must pass.

## 7. Continuous integration (GitHub Actions)

| Job | Runner | Steps |
|---|---|---|
| `linux-coverage` | Ubuntu 24.04 (GCC) | Build `coverage`; C, Lua, integration, scenario, determinism and golden tests under Xvfb + llvmpipe; gcovr and Lua coverage gates |
| `linux-sanitize` | Ubuntu 24.04 (Clang) | Build `sanitize`; all tests |
| `windows` | Windows 2022 (MSVC) | Build `release`; C, Lua, integration and scenario tests (headless) |
| `macos` | macOS 14 (arm64, AppleClang) | Build `release`; C, Lua, integration and scenario tests (headless) |
| `web` | Ubuntu 24.04 + emsdk | Build `web-release`; Playwright desktop, mobile and XR-mock tests |
| `checks` | Ubuntu 24.04 | clang-format, clang-tidy, asset checks, `openspec validate --strict` |
| `package` | All | Archive per platform: executable + `data/` + licenses; web bundle (`.html/.js/.wasm/.data`) |

A pull request can only be merged when every job is green.

## 8. Build system

- **CMake ≥ 3.24** with **presets** (`CMakePresets.json`):

| Preset | Purpose |
|---|---|
| `dev` | Debug build, developer mode (console, hot reload) |
| `release` | Optimized build, developer mode off by default |
| `coverage` | GCC + `--coverage`, tests on |
| `sanitize` | Clang + ASan/UBSan, tests on |
| `web-dev` / `web-release` | Emscripten builds; `.data` file packed from `data/` |

- Options: `PRTS_BUILD_TESTS`, `PRTS_USE_SYSTEM_SDL3`, `PRTS_WEB_THREADS` (off by default).
- **Output layout** is the same on every platform: `build/<preset>/bin/1stprts(.exe)` with a
  `data/` folder **copied or linked next to it** at build time, so the build output runs as is.
- **C standard**: C11, plus the extras needed for SDL3 and atomics. Compilers: GCC ≥ 11,
  Clang ≥ 14, MSVC 2022, Emscripten (current stable).

## 9. Dependencies

| Library | Use | License | How it is included |
|---|---|---|---|
| SDL 3 | Window, GL context, input, audio, threads, paths | zlib | CMake FetchContent pinned to a release tag (or system package) |
| Lua 5.4 | Scripting | MIT | Vendored source |
| cgltf | glTF 2.0 loading | MIT | Vendored single header |
| stb_image, stb_vorbis | PNG, OGG | MIT or public domain | Vendored single headers |
| ogt_vox | MagicaVoxel `.vox` loading | MIT | Vendored single header |
| microui | Immediate-mode UI core (menus, console), wrapped by `prts_ui` | MIT | Vendored source |
| utest.h | C unit-test framework | Unlicense | Vendored (tests only) |
| OpenGL loader | GL function pointers via `SDL_GL_GetProcAddress` | MIT (our code) | Generated from a list of the functions we use |
| WebXR glue | XR session, poses, layer | MIT (our code) | `web/` JS library for Emscripten |
| Playwright | Web end-to-end tests | Apache-2.0 | npm dev dependency |
| gcovr | Coverage reports | BSD-3-Clause | CI tool (pip) |
| OpenSpec | Spec workflow | MIT | npm dev tool |

Each vendored library keeps its license file in `third_party/<lib>/`. The game's Credits screen
lists third-party code as well as art.

## 10. Definition of done (every OpenSpec change)

1. Every box in the change's `tasks.md` is checked.
2. `openspec validate --strict` passes.
3. All CI jobs are green on all platforms.
4. Coverage is **≥ 95%** for both C and Lua.
5. Each new requirement scenario has at least one automated test (unit, scenario, golden or E2E).
6. Docs and credits are updated, and new assets have `CREDITS.md` entries.
7. The change is archived, so its specs merge into `openspec/specs/`.
