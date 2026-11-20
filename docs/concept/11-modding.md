# 11 · Modding

**Goal:** the base game is just the first mod. Anything it does, a mod can do too, including a
new faction, a new people or a new kind of magic.

## 1. What can be modded

| Area | How | Files |
|---|---|---|
| Units, buildings, weapons, spells, factions | Lua definition tables | `scripts/defs/**.lua` |
| Rules, victory conditions, economy, tiers and upgrades, presence, succession | Lua | `scripts/game/**.lua` |
| AI (captains and the opposing commander) | Lua | `scripts/ai/**.lua` |
| Command interfaces (field map, war table, scrying pools, menus) | Lua UI scripts | `scripts/ui/**.lua` |
| Missions, campaigns, scenarios | Lua | `scripts/missions/**.lua` |
| Interiors (castles, mines, ships, behemoths, citadels) | Deck plans (text) and hero-room models | `interiors/**` |
| Maps | Heightmaps, splat maps, placement scripts | `maps/**` |
| Models, textures, sprites, voxels | glTF 2.0, PNG, sprite sheets with metadata, `.vox` | `models/`, `textures/`, `sprites/`, `voxels/` |
| Sound, music, voice phrases | WAV, OGG, phrase tables | `sounds/`, `music/`, `voices/` |
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
    sounds/ music/ voices/ fonts/
    interiors/ maps/
mods/                  extra mods (a second mods folder lives in the user data directory)
```

On the **web**, the `data/` folder is packed into a separate data file that is downloaded next to
the `.wasm` binary. It is not compiled into the binary, and it can be swapped without rebuilding.

## 3. Mod manifest

```lua
-- data/mods/heavy_cavalry/mod.lua
return {
  id          = "heavy_cavalry",
  name        = "Heavy Cavalry Pack",
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
-- scripts/defs/units/crown_knights.lua
prts.def.unit {
  id         = "crown.knights",
  name       = "Knights",
  faction    = "crown",
  domain     = "field",
  members    = 4,                               -- one lance of four riders
  sprite     = "sprites/crown/knight_mounted.lua",
  crew       = { { role = "knight", count = 4 } },
  mobility   = { type = "mounted", max_speed_kmh = 40, stamina = 120 },
  armor      = { melee = 8, pierce = 6, fire = 2 },
  weapons    = {
    { weapon = "crown.lance", charge_bonus = 2.0 },
    { weapon = "crown.longsword" },
  },
  sight_m    = 300,
  upgrades   = { "crown.lantern_knights" },
  trained_at = "crown.stables",
  cost       = { gold = 250, timber = 60, folk = 4, train_time_s = 70 },
}
```

## 5. Example: a deck plan

```lua
-- interiors/crown/war_galleon/gun_deck_lower.lua
prts.def.deckplan {
  id     = "crown.war_galleon.gun_deck_lower",
  cell   = 2.0,  height = 2.0,
  kit    = "kits/crown_ship",
  legend = {
    ["#"] = "hull", ["."] = "deck", ["+"] = "hatch",
    ["^"] = "ladder_up", ["v"] = "ladder_down",
    ["g"] = "cannon", ["k"] = "powder_kegs", ["b"] = "hammock",
  },
  tiles = [[
    ###################
    #ggg..g#kkk#bb.bb.#
    #......+.k.+......#
    #ggg..g#...#bb.bb.#
    ####+#########+####
    #^...............v#
    ###################
  ]],
  rooms = [[
    ###################
    #GGGGGG#PPP#BBBBBB#
    #GGGGGG+PPP+BBBBBB#
    #GGGGGG#PPP#BBBBBB#
    ####+#########+####
    #WWWWWWWWWWWWWWWWW#
    ###################
  ]],
  zones = {
    G = { name = "Forward Battery", system = "guns", hero_room = "battery_fwd" },
    P = { name = "Powder Room",     system = "guns.powder", fire_risk = "high" },
    B = { name = "Crew Berths",     system = "crew" },
    W = { name = "Main Passage" },
  },
}
```

From this one file the engine builds the geometry from the kit, the collision, the crew
navigation graph and the rooms used for fire and flooding.

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
