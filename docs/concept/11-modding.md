# 11 · Modding

**Goal:** the base game is just the first mod. Anything it does, a mod can do too.

## 1. What can be modded

| Area | How | Files |
|---|---|---|
| Units, structures, weapons, factions | Lua definition tables | `scripts/defs/**.lua` |
| Rules, victory conditions, economy, presence, succession | Lua | `scripts/game/**.lua` |
| AI (officers and the opposing commander) | Lua | `scripts/ai/**.lua` |
| Command interfaces (slate, table, monitors, menus) | Lua UI scripts | `scripts/ui/**.lua` |
| Missions, campaigns, scenarios | Lua | `scripts/missions/**.lua` |
| Interiors | Deck plans (text) and hero-room models | `interiors/**` |
| Maps | Heightmaps, splat maps, placement scripts | `maps/**` |
| Models, textures, sprites, voxels | glTF 2.0, PNG, sprite sheets with metadata, `.vox` | `models/`, `textures/`, `sprites/`, `voxels/` |
| Sound, music, radio phrases | WAV, OGG, phrase tables | `sounds/`, `music/`, `radio/` |
| The look | GLSL shaders, palettes | `shaders/`, `palettes/` |
| Controls | Action definitions, default bindings | `scripts/input/**.lua` |

The engine does not contain game content. If an asset or a rule is missing from `data/`, the
game does not have it.

## 2. Folder layout next to the executable

```
1stprts(.exe)          the engine, plus a small launcher
data/
  base/                the base game (a mod like any other)
    mod.lua            manifest
    CREDITS.md         per-asset attribution (CC BY 4.0)
    scripts/           Lua (MIT)
    shaders/           GLSL (MIT)
    models/ textures/ sprites/ voxels/ palettes/
    sounds/ music/ radio/ fonts/
    interiors/ maps/
mods/                  extra mods (a second mods folder lives in the user data directory)
```

On the **web**, the `data/` folder is packed into a separate data file that is downloaded next to
the `.wasm` binary. It is not compiled into the binary, and it can be swapped without rebuilding.

## 3. Mod manifest

```lua
-- data/mods/heavy_armor/mod.lua
return {
  id          = "heavy_armor",
  name        = "Heavy Armor Pack",
  version     = "1.2.0",
  api         = "1",                 -- engine scripting API major version
  depends     = { "base >= 0.1" },
  load_after  = { "base" },
  authors     = { "Jane Modder" },
  license     = { code = "MIT", art = "CC-BY-4.0" },
}
```

- Mods are **mounted in load order**. A later mod's file **overrides** the same path from an
  earlier one, and a mod can add new files.
- Definitions are **merged by id**. A mod can change a single field of a base unit without
  copying the whole definition.
- An incompatible `api` version gives a clear error, and the game keeps running.

## 4. Example: a unit definition

```lua
-- scripts/defs/units/dir_mbt_bastion.lua
prts.def.unit {
  id        = "dir.mbt.bastion",
  name      = "Bastion Main Battle Tank",
  faction   = "dir",
  domain    = "ground",
  size      = { 3.6, 2.6, 10.0 },         -- width, height, length in meters
  model     = "models/dir/mbt_bastion.glb",
  interior  = "interiors/dir/mbt_bastion/interior.glb",
  crew      = {
    { role = "commander", station = "turret.commander" },
    { role = "gunner",    station = "turret.gunner" },
    { role = "loader",    station = "turret.loader" },
    { role = "driver",    station = "hull.driver" },
  },
  mobility  = { type = "tracked", max_speed_kmh = 60, fuel_l = 1400 },
  armor     = { front = 600, side = 250, rear = 120, top = 80 },
  weapons   = { { weapon = "dir.gun.120mm", mount = "turret.main", ammo = 40 } },
  sensors   = { { type = "optics", range_m = 4000 } },
  subsystems = {
    propulsion = { compartments = { "engine" } },
    weapons    = { compartments = { "turret" } },
    comms      = { compartments = { "turret" } },
  },
  cost      = { alloy = 180, fuel = 40, build_time_s = 90 },
}
```

## 5. Example: a deck plan

```lua
-- interiors/dir/destroyer_kestrel/deck2.lua
prts.def.deckplan {
  id     = "dir.destroyer.kestrel.deck2",
  cell   = 2.0,  height = 2.8,
  kit    = "kits/dir_naval",
  legend = {
    ["#"] = "wall", ["."] = "floor", ["+"] = "hatch",
    ["^"] = "ladder_up", ["v"] = "ladder_down", ["c"] = "console", ["b"] = "bunk",
  },
  tiles = [[
    ###################
    #ccc..c#ccc#bb.bb.#
    #......+.c.+......#
    #ccc..c#...#bb.bb.#
    ####+#########+####
    #^...............v#
    ###################
  ]],
  compartments = [[
    ###################
    #AAAAAA#RRR#BBBBBB#
    #AAAAAA+RRR+BBBBBB#
    #AAAAAA#RRR#BBBBBB#
    ####+#########+####
    #PPPPPPPPPPPPPPPPP#
    ###################
  ]],
  zones = {
    A = { name = "Combat Information Center", subsystem = "command", hero_room = "cic" },
    R = { name = "Radar Room",                subsystem = "sensors.radar" },
    B = { name = "Berthing 2-1",              subsystem = "crew" },
    P = { name = "Main Passageway" },
  },
}
```

The engine builds the geometry from the kit, the collision, the crew navigation graph and the
compartments for damage control from this one file.

## 6. Scripting rules

- Scripts run in a **sandbox**. There is no direct access to the operating system or files.
  Data is read through the engine's virtual file system, and saving goes through a save API.
- **Deterministic**: scripts get the simulation clock and seeded random numbers. Wall-clock time
  and OS random sources are not available in simulation code.
- **Errors do not crash the game**. A failing script is reported in the developer console and log
  with file and line. In development mode the game keeps running, and the scripts can be fixed
  and **hot-reloaded**.
- **Performance budget**: each simulation tick has a script time budget. AI thinking is spread
  across ticks.
- **Tests**: mods can ship Lua unit tests and scenario tests and run them with the same test
  runner as the base game (see [architecture/quality-and-build.md](../architecture/quality-and-build.md)).

## 7. Tools (roadmap)

- **In-game developer console** (M1): run Lua, reload scripts, inspect entities.
- **Asset validator** (M1): checks manifests, credits coverage, references and formats.
- **Deck-plan preview** (M3): walk through a deck plan while editing it, with live reload.
- **Map and scenario editor** (M8): a first-person editor in the engine, plus text formats.
