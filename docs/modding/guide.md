# Modding guide

*Banner & Bastion* is built to be modded: everything the game plays is Lua scripts and plain data
under `data/base/`, and a mod is a folder that adds to it or replaces parts of it. This guide
walks through the example mod in [`examples/mods/watchfire/`](../../examples/mods/watchfire/),
then covers maps, units, languages, the editor's maps and tests. The full list of engine
functions is the [scripting API reference](api-m1.md). The concept behind it is
[concept 11](../concept/11-modding.md).

## 1. A mod's folder

```
watchfire/
  mod.lua                          the manifest
  CREDITS.md                       every art file and its license
  scripts/mods/watchfire/init.lua  the entry point, run at boot
  scripts/maps/watchfire_hill.lua  a map
  scripts/lang/fr.lua              a language
```

**Where mods live.** Put the folder in the `mods` folder of the user data folder: on desktops
the folder the log names at start-up, on the web the browser's storage. Enable it with
`--mod watchfire` (once per mod).

**What a mod sees.** The mod's files sit over the base game's in one merged file system: a mod
file with the same path as a base file replaces it, and a new path adds to the game. Mods load
after the base game in dependency order.

**The manifest** (`mod.lua`) is plain data:

```lua
return {
  id      = "watchfire",       -- the folder's name
  name    = "Watchfire Hill (example mod)",
  version = "1.0.0",
  api     = "1",               -- the scripting API version it targets
  depends = { "base >= 1.0" }, -- other mods (and the base game) it needs
}
```

The manifest's fields are described in the reference under "Mod manifests".

## 2. The entry point

At boot the base game runs every enabled mod's `scripts/mods/<id>/init.lua`, in load order,
after its own modules and before the first map. That is where a mod adds things.

- **Use your own path.** The path holds the mod's id, so mods never replace each other's entry
  points.
- **Errors do not stop the game.** A failing entry point is logged ("mod watchfire: ...") and
  the game carries on.

```lua
local session = require("game.session")
local D = require("rts.defs")
local I = require("lib.i18n")

table.insert(session.maps, { id = "watchfire_hill", name = "Skirmish: Watchfire Hill (example mod)" })
table.insert(D.UNITS, { id = "watchfire.lantern_guard", name = "Lantern guard", sprite = "guard", ... })
D.HORN_RANGE = 2600
I.add("de", { ["Lantern guard"] = "Laternenwache" })
```

**Avoid `scripts/main.lua`.** It is the base game's boot script: replacing it replaces the
whole start-up. Prefer the entry point.

## 3. A map

A map is a module `scripts/maps/<id>.lua`. `game/session.lua` loads it when the map starts:

1. It calls `build()`, which defines materials, the terrain, light and sky, places what the map
   holds, and sets the `start` marker.
2. It spawns the commander at the marker and finishes the scene.
3. It emits `session_started` with the map's id; the map's handler then builds the navigation
   grid and starts its match.

```lua
function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -480, -480 }, max = { 480, 480 }, cell = 2 }
  require("rts.match").start{ sides = M.SIDES, serfs = 4, custodians = "neutral", ruins = M.RUINS }
  return true
end
prts.events.on("session_started", "maps.watchfire_hill", M.on_started)
```

**Adding a map to a menu.** Add it to `session.maps` and it is offered on the Battles screen.
The Watchfire map is a complete example, about 80 lines. The base game's maps (`ashford_vale`,
`thornmere`, `tidesend`, ...) show more: harbours, citadels, tunnels and neutral towns.

**The First-Person Contract.** Gameplay is seen only through the commander's eyes. Maps and
mods must not add overhead, free or third-person cameras (the developer observer camera exists
only in developer builds).

## 4. Units, buildings and rules

**Definitions are data.** Factions are plain tables in `rts/defs.lua`, `rts/wildborn.lua`,
`rts/gloam.lua`, `rts/brine.lua` and `rts/custodian_defs.lua`:
- `D.UNITS`, `D.STRUCTURES`, `D.SHIPS`, `D.UPGRADES`, `D.MANDATES` and `D.SPELLS`;
- `D.FACTIONS[f]`, the role table and the AI's plans.

**Adding.** Append to the lists, or change a field of an existing definition. They are
registered (`prts.def`) when a match starts.

**How a unit is drawn.**
- `sprite` names a sheet coloured by side (`crown_guard`, `red_guard`);
- `sheet` names a faction's own look;
- `model` with `voxel` draws it from a MagicaVoxel model, as the Custodians are.

**What a unit can do.** `at` names the structure that trains it, and `tier` the tier it needs.
`weapon` holds `damage`, `range`, `cooldown`, `missile`, `flyers`, `ships` and `structures`.
The flags are `flying`, `caster` (with `spells`), `worker` and `mounted`.

**Match rules** are constants on `D`, such as `D.HORN_RANGE`, `D.MINE_GOLD` and `D.WORK`.

**A new faction** needs its definitions and a `D.FACTIONS` entry with its roles (`hq`, `worker`,
`food`, `foot`, ...) and an `ai` plan. `rts/brine.lua` is the most recent full example.

## 5. Languages

Text is translated by `lib/i18n.lua`, keyed by the English text itself.

**Adding a language.** Add `scripts/lang/<code>.lua`. It is found and offered on the options'
Game tab:

```lua
return {
  name = "Français",
  strings = { ["Quit"] = "Quitter", ["Options"] = "Options" },
  patterns = { { "^Continue: (.+)$", "Continuer : %1" } },
}
```

- **Strings** are exact English lines.
- **Patterns** match composed lines; their captures are translated in turn, so an act's title
  inside "Continue: ..." is translated too.

**Adding to a language.** `I.add(code, strings, patterns)` adds strings to a language, for
example German for your mod's own words.

**What is translated.**
- Everything the UI draws is translated as it is drawn.
- Widget ids stay English, so tests and key bindings never depend on the language.
- Untranslated text shows in English.

**The font** has the Latin-1 letters (é, ü, ñ, ß, ...); write the files as UTF-8.

## 6. Maps made with the editor

The in-game map editor (main menu, Map editor) saves maps as plain text files in the user
folder's `files/` (`prts.files`). One instruction per line:

```
format 1
name Two Lords
size 1024
seed 42
hills 12
time dusk
start -58 14 0
place tree pine 20 20
place mine gold 30 -20 0
place spring aether -20 40 0
place ruin imperium 0 120 0
place prop crate -55 -12 0.79
place holdfast crown -120 0 0 1
place holdfast red 120 0 0 2
place company crown.men_at_arms -100 20 0 1
```

| Instruction | Meaning |
|---|---|
| `format 1` | The format's version; required. |
| `name <text>` | The map's name (the rest of the line). |
| `size <m>` | The terrain's width, 256–2048 m. |
| `seed <n>` | The terrain's random seed. |
| `hills <m>` | How high the hills rise, 0–40 m. |
| `time day\|dusk\|night` | The light and sky. |
| `start x z yaw` | Where the commander starts. |
| `place <kind> <id> x z [yaw [side]]` | An object. Kinds: `tree`, `mine`, `spring`, `ruin` (held by the Custodians when played), `prop` (`crate`, `anvil`, `brazier`, `bench`, `banner_stand`), `holdfast` (`crown`, `red`, `wildborn`, `gloam`, `brine`; one per side, side 1 the player's) and `company` (six of a unit, for a side). |

**Reading a map file.** The file is read as data, never run. A broken line is reported as
"line N: why", and the map then plays empty.

**Playing it.** Two or more holdfasts make a match. The editor shows the same map with stand-ins
the commander walks among:
- place with the left mouse button or P;
- remove with Delete;
- pick with `]` and `[`;
- turn with T.

## 7. Credits and licenses

- **Code** is MIT.
- **Art** (textures, models, sounds, music, sprites) must be CC BY 4.0 or compatible, and every
  art file needs a row in the mod's `CREDITS.md`: title, author, license, source and changes.

The credits screen lists every enabled mod's credits. The base game's assets are checked with
`tools/validate_assets.py`; run it on your own pack the same way.

## 8. Testing a mod

Mods use the same test runner as the base game:

- **Lua unit tests:** `scripts/tests/test_*.lua` with `T.case`, run with `--test-lua`.
- **Scenario tests:** a file returning `{ name, max_seconds, run = function(t) ... end }` that
  drives the game through real input. Run it with
  `1stprts --headless --mod <id> --start <map> --scenario <file>`.

`tests/game/scenarios/example_mod.lua` is the scenario that checks the Watchfire mod in CI. The
full `t` and `T` APIs are in the [reference](api-m1.md#scenario-tests-t).

**Determinism.** The simulation is deterministic: use `prts.time` and `math.random` (the seeded
stream), never the wall clock, so replays and tests give the same result every time.

## 9. Where to look next

- **[The scripting API](api-m1.md):** every engine function, and the base game's modules for
  factions, ships, citadels, tunnels, the Custodians, the campaign, the editor and localization.
- **The campaign:** `game/campaign.lua`. Acts are data, so a mod can add or replace acts with
  its own maps.
- **Story and dialogue:** `story/harrowmark.lua`, people with `props/npc.lua`.
