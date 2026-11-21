# Data & Scripting

## 1. Folders next to the binary

```
<install dir>/
  1stprts(.exe)        engine + launcher
  data/
    base/              base game (mod id "base")
  mods/                extra mods shipped with or added to an install

<user data dir>/       from SDL_GetPrefPath("1stPRTS", "1stprts")
  config.lua           settings and key bindings (written by the game)
  saves/  replays/  logs/  screenshots/
  mods/                mods installed by the user
```

- The install directory is found with `SDL_GetBasePath()`. On macOS app bundles the `data/`
  folder sits in `Contents/Resources/`, which counts as "beside the binary" for a bundle.
- Command-line overrides: `--data <dir>` (add or replace the data root), `--mod <id>`
  (enable a mod), `--user <dir>` (user data directory, which tests use for isolation).
- **Web**: `1stprts.html`, `1stprts.js`, `1stprts.wasm` and **`1stprts.data`** (the packed
  `data/` folder, loaded by Emscripten's file packager into an in-memory file system at
  `/data`). The data file is a separate download (with its small loader `1stprts.data.js`, which
  the shell page loads first) and can be rebuilt without rebuilding the `.wasm`. The user data
  directory on the web is IndexedDB-backed (`/user`, IDBFS): it is loaded before the engine
  starts and every change is written back after the frame that made it (IDBFS `autoPersist`), so
  settings and saves are kept between visits. See `web/` and `cmake/PrtsWeb.cmake`.

## 2. Virtual file system (VFS)

- **Mount table**: an ordered list of roots (`data/base`, then each enabled mod in load
  order). A lookup checks mounts from the **last to the first**, so later mods override earlier
  ones.
- **Paths** are virtual, case-sensitive, use `/`, and are relative (e.g. `models/dir/mbt.glb`).
  Absolute paths, `..`, backslashes and drive letters are **rejected**. The same rules apply on
  every platform, so a mod that works on Linux works on Windows and on the web.
- **Read-only** for game content. Writing is only allowed in the user area (`user:/saves/…`)
  through dedicated APIs.
- **Listing**: a directory listing merges all mounts, which is how definitions are found
  (for example "load every file in `scripts/defs/units/`").
- **Archives**: plain directories in M1. Zip mounts are a later addition behind the same
  interface.

## 3. Asset pipeline

| Type | Format(s) | Loader | Notes |
|---|---|---|---|
| Textures | PNG | stb_image | Paletted or RGBA, power-of-two not required |
| Models | glTF 2.0 (`.glb`, `.gltf`) | cgltf | Subset; marker nodes for seats, doors, portals and volumes |
| Voxels | MagicaVoxel `.vox` | ogt_vox | Greedy-meshed when loaded |
| Sprites | PNG sheet + Lua metadata | engine | Directions, animations, frame timing, pivot |
| Audio | WAV, OGG Vorbis | SDL3 / stb_vorbis | Music is streamed; sound effects are decoded up front |
| Fonts | PNG atlas + Lua metrics | engine | Bitmap fonts |
| Shaders | GLSL (shared dialect) | engine | Platform prelude added when loaded |
| Palettes | PNG strip (256 colors) | engine | Converted to a 3D lookup texture |
| Deck plans | Lua (see concept 11) | script + engine | Compiled into geometry and data (M3) |
| Maps | Heightmap PNG (16-bit) + splat PNG + Lua placement | engine + script | |

- Assets are **referenced by path** and loaded through the **asset registry**, which returns
  **handles**. Loads are **asynchronous**: decoding runs as jobs, and GPU upload happens on the
  main thread. Missing or broken assets are replaced by a visible **"missing" placeholder**
  (magenta checker, beep sound), and an error is logged. The game does not crash.
- **Hot reload** (developer mode): file changes under mounted roots trigger a reload of the
  matching asset, and every handle sees the new version. On the web, hot reload is off.

## 4. Lua runtime

- **Lua 5.4**, vendored and built as C. It runs the same on desktop and WASM. It is built with a
  fixed hash seed for determinism ([simulation.md §6](simulation.md#6-determinism)).
- **One VM** hosts the base game and all mods. Each mod's files run in their own environment
  table that inherits from a shared, read-only global environment.
- **Boot**: the engine runs `scripts/main.lua` from the VFS (the last mount that provides it
  wins). From there, scripts load definitions, register systems and event handlers, and start
  the front-end menu or the scenario named on the command line.
- **`require`** is replaced by a VFS-aware loader: `require("game.rules")` loads
  `scripts/game/rules.lua` through the mount table.

### API layout (`prts.*`)

| Namespace | Purpose |
|---|---|
| `prts.api_version`, `prts.engine_version`, `prts.platform` | Version and platform info (desktop, web, web-mobile, xr) |
| `prts.log` | Logging at levels, shown in the developer console |
| `prts.vfs` | Read-only file access, directory listing (virtual paths) |
| `prts.def` | Register definitions (`unit`, `structure`, `weapon`, `faction`, `deckplan`, `action`…) merged by id |
| `prts.entity` | Create and destroy entities, get and set components, find entities, attach script data |
| `prts.frame` | Reference frames: parent, local and world transforms, frame volumes |
| `prts.events` | Subscribe to and emit events (`tick`, `use`, `door_opened`, `entered_cell`, …) |
| `prts.time` | Tick, simulation seconds, timers |
| `prts.rand` | Seeded random streams |
| `prts.input` | Action definitions, action states, input contexts, the active scheme (kbm, gamepad, touch, xr) |
| `prts.audio` | Play positional sounds, music, set bus volumes |
| `prts.ui` | Immediate-mode UI for screens and world surfaces |
| `prts.render` | Render settings (retro preset, effects), far-view surfaces (scrying pools) |
| `prts.scene` | Load a map or scene, spawn prefabs |
| `prts.save` | Save and load slots (user area) |
| `prts.test` | Test helpers: assertions, simulated input, running ticks (only in test mode) |
| `prts.sorted_pairs` | Iteration in key order, for deterministic simulation code |

### Binding conventions
- Engine objects are **handles** (integers). Every call checks them, and invalid handles raise
  `prts.error.stale_handle`.
- Functions take **tables for options**: `prts.audio.play{ sound = "sounds/door.wav", at = e }`.
- Errors are Lua errors with clear messages. The engine catches them at every C→Lua entry
  point.
- Callbacks are registered by name (`prts.events.on("use", fn)`) so they can be swapped on hot
  reload.

### Sandbox
- **Removed**: `os.execute`, `os.exit`, `os.remove`, `os.rename`, `os.tmpname`, `os.getenv`,
  `io.*`, `loadfile`, `dofile`, `package.loadlib`, `package.cpath`, the full `debug` library,
  `collectgarbage("stop")`.
- **Replaced**: `require` (VFS loader), `print` (goes to `prts.log`), `math.random` and
  `math.randomseed` (simulation RNG stream), `load` (text chunks only, never bytecode).
- **Budgets**: an instruction-count hook checks each script entry point against its time budget.
  A script that goes over the hard limit (for example an endless loop) is interrupted with an
  error. The game keeps running.

### Hot reload (developer mode)
1. A changed `.lua` file is detected through the VFS.
2. The module is re-run in its environment. Definitions are merged again by id, and event
   handlers registered by name are replaced.
3. **State in entities' script data is kept**, so reloading never resets the world.
4. `scripts/main.lua` can define `on_reload()` for its own fix-ups.

## 5. Settings and bindings

- `config.lua` in the user area is a plain Lua data table (loaded in a data-only sandbox). It
  holds video (retro preset, effects), audio volumes, input bindings per scheme, accessibility
  options and developer flags.
- Defaults come from `data/base/scripts/input/defaults.lua`. The user file stores only
  overrides.
