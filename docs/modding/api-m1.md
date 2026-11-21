# Scripting API (M1)

This is the Lua API the engine offers to the base game and to mods in milestone M1
(scripting API version **1**). Every function the engine registers is listed here;
`tools/check_api_docs.py` (run in CI) fails if one is missing. The architecture behind it is in
[`../architecture/data-and-scripting.md`](../architecture/data-and-scripting.md).

Conventions:

- Engine objects are **handles** (integers). Functions that take an entity check it and raise a
  `stale entity handle` error for a destroyed one, so scripts never crash the engine.
- Functions that take many options take one table: `prts.scene.box{ min = ..., max = ... }`.
- A vector is `{ x, y, z }` or `{ x = ..., y = ..., z = ... }`. Positions are in meters, angles in
  radians; yaw 0 looks towards -z (north) and positive yaw turns left (towards -x).
- Errors are ordinary Lua errors. The engine catches them at every call into Lua, logs them with a
  traceback, and keeps running.

## Environment and sandbox

Each mod's files run in their own environment table, which reads through to a shared global
table. The standard libraries `string`, `table`, `math`, `utf8` and `coroutine` are available, plus
the base functions except `dofile`, `loadfile` and `collectgarbage`. `os`, `io`, `package` and
`debug` do not exist.

| Name | What it does |
|---|---|
| `require(name)` | Loads `scripts/<name with dots as slashes>.lua` from the highest-priority mount, once, and returns what it returned (`true` if nothing). |
| `load(text [, chunkname])` | Compiles Lua **source text** only (never bytecode). Returns the function, or `nil` and the error. |
| `print(...)` | Writes its arguments to the log at info level (same as `prts.log.info`). |
| `math.random([m [, n]])`, `math.randomseed(n)` | The seeded simulation random stream (the same as `prts.rand.random` / `prts.rand.seed`). |

## Version and platform

| Name | What it is |
|---|---|
| `prts.api_version` | Major version of this API (integer, `1`). Mods state the version they target in `mod.lua`. |
| `prts.engine_version` | Engine version string, for example `"0.1.0"`. |
| `prts.platform` | `"desktop"` or `"web"`. |
| `prts.sorted_pairs(t)` | Like `pairs`, but in key order: numbers ascending, then strings ascending (other keys are skipped). Use it wherever iteration order affects the simulation. |

## `prts.log`

| Function | What it does |
|---|---|
| `prts.log.debug(...)` | Logs its arguments (converted with `tostring`, tab-separated) at debug level. |
| `prts.log.info(...)` | Same, info level. |
| `prts.log.warn(...)` | Same, warning level. |
| `prts.log.error(...)` | Same, error level (counts as an error in test runs). |

## `prts.time` and `prts.rand`

Scripts get time only from the simulation clock and random numbers only from seeded streams.

| Function | What it does |
|---|---|
| `prts.time.tick()` | The current simulation tick (30 ticks per second). |
| `prts.time.seconds()` | Simulated seconds: tick × tick length. |
| `prts.time.dt()` | The length of one tick in seconds (1/30). |
| `prts.rand.random([m [, n]])` | Like `math.random`: no argument gives a float in [0, 1), one gives an integer in [1, m], two give an integer in [m, n]. |
| `prts.rand.seed(n)` | Restarts the stream from seed `n`. |

## `prts.vfs`

Read-only access to the virtual file system (all mounts merged; paths are relative, use `/` and
are case-sensitive; `..`, `\`, drive letters and absolute paths are refused).

| Function | What it does |
|---|---|
| `prts.vfs.read(path)` | The file's contents as a string, or `nil` and a reason (`"not found"`, `"invalid path"`). |
| `prts.vfs.exists(path)` | `true` if a mount has the file. |
| `prts.vfs.list([dir])` | Sorted array of `{ name = ..., dir = bool }` entries, merged over all mounts (`""` or nothing for the root). Errors on an invalid path. |

## `prts.events`

Handlers are registered **by name**, so a module re-run by hot reload replaces its handlers
instead of adding more. Handlers for an event run in name order; an error in one does not stop the
others.

| Function | What it does |
|---|---|
| `prts.events.on(event, name, fn)` | Registers (or replaces) handler `name` for `event`. |
| `prts.events.off(event, name)` | Removes it. |
| `prts.events.emit(event, ...)` | Calls every handler of `event` with the arguments. Returns how many failed. |

Events sent by the engine:

| Event | Arguments | When |
|---|---|---|
| `tick` | tick number | After each simulation tick's systems ran. |
| `use` | entity, entity name | The commander pressed `use` while looking at a usable entity within reach. |
| `move_done` | entity, entity name | A move started with `prts.entity.move_to` or `rotate_to` finished. |

## `prts.def`

Definitions (units, buildings, weapons, factions, actions, ...) registered by **kind** and **id**.
Defining an id that exists **merges** the new table into the old one, so a mod can change one field
of a base definition: tables merge key by key, while arrays (sequences `1..n`) and plain values
replace the old value; an empty table leaves the old value unchanged. Definitions must be plain data
(numbers, strings, booleans and tables of them); anything else is refused with an error naming the
field.

```lua
prts.def.define("unit", { id = "archer", hp = 40, weapon = { damage = 6, range = 60 } })
prts.def.define("unit", { id = "archer", weapon = { range = 80 } })   -- a mod: only the range changes
prts.def.get("unit", "archer").weapon.damage                          --> 6
```

| Function | What it does |
|---|---|
| `prts.def.define(kind, tbl)` | Registers or merges `tbl` (it needs a string `id`). Returns `true` if the id was new. The registry keeps its own copy. `kind` uses letters, digits, `_` and `.`. |
| `prts.def.get(kind, id)` | A copy of the definition, or `nil`. Changing the copy does not change the definition. |
| `prts.def.list(kind)` | Sorted array of the ids of a kind (empty if none). |
| `prts.def.kinds()` | Sorted array of the kinds that have definitions. |
| `prts.def.<kind>{ ... }` | Shorthand for `prts.def.define("<kind>", { ... })`, for example `prts.def.unit{ id = "crown.knights", ... }`. Any kind name works except `define`, `get`, `list` and `kinds`. |

## `prts.scene`

Building the map: materials, greybox geometry, terrain, lights, markers and zones. Geometry owned
by an entity (`entity = e`) is in that entity's frame and moves with it.

| Function | What it does |
|---|---|
| `prts.scene.material{ id, texture, tint, uv_scale, emissive }` | Defines a material (`uv_scale` default 2, `emissive` default false). Returns its index. |
| `prts.scene.box{ min, max, material, entity, visible, solid, tile }` | An axis-aligned box: drawn unless `visible = false`, solid unless `solid = false`. Returns its collision box id, or -1. |
| `prts.scene.prism{ base, radius, top_radius, height, sides, material, entity, solid }` | A prism or cone (tree trunks, roofs, towers); solid only with `solid = true`. |
| `prts.scene.terrain{ size, spacing, seed, height, octaves, material, flats }` | Generates the terrain height field; `flats` is a list of `{ x, z, radius, height, blend }` areas levelled for buildings. |
| `prts.scene.height(x, z)` | Terrain height at a point. |
| `prts.scene.light{ pos, color, radius }` | A point light (baked into vertex lighting). |
| `prts.scene.marker(name, pos [, yaw])` | A named position (spawn points, test markers). |
| `prts.scene.marker_pos(name)` | `x, y, z, yaw` of a marker, or nothing. |
| `prts.scene.zone(name, min, max)` | A named box used by tests and scripts to ask "is it inside?". |
| `prts.scene.env{ sun_dir, sun_color, ambient, fog_color, sky_horizon, sky_zenith, fog_near, fog_far, stars, moons }` | Lighting, fog and sky. |
| `prts.scene.illusion{ center, size_x, size_z, relief, levels, area, color, entity }` | A glowing contour-line model of the terrain in `area = { x0, z0, x1, z1 }`, for the war table. |
| `prts.scene.cell{ name, min, max }` | An interior cell (a room or group of rooms). Geometry added afterwards belongs to the cells set with `prts.scene.cells`. At most 63 cells; `"outside"` always exists. |
| `prts.scene.cells{ name, ... }` / `prts.scene.cells()` | Geometry added from now on belongs to these cells (drawn when any of them is visible); no argument means "everywhere". |
| `prts.scene.cell_at(x, y, z)` | Name of the smallest cell containing the point, or `"outside"`. |
| `prts.scene.portal{ name, a, b, min, max, open }` | An opening between cells `a` and `b` (default `"outside"`): a door, an archway or a window. `open` defaults to true. Visibility and sound pass only through open portals. |
| `prts.scene.set_portal(name, open)` | Opens or closes a portal (doors call this). Errors on an unknown name. |
| `prts.scene.portal_open(name)` | `true` if the portal is open. |
| `prts.scene.surface{ name, pos, yaw, pitch, fov, size, fog }` | A camera that renders the scene into a texture every frame (`size = { w, h }` pixels, `fov` in degrees, `fog` default false). A material with `surface = name` shows it, for example the war table. |
| `prts.scene.sprite{ sprite, pos, yaw, entity, anim }` | A billboard person or creature from a sprite sheet description (`sprites/<name>.lua`: `texture`, `frame = { w, h }`, `columns`, `rows`, `height`, `directions`, `anims = { { name, first_row, frames, fps } }`). It shows the frame for the viewing angle (8 directions) and follows `entity` if given. Returns the sprite id. |
| `prts.scene.sprite_anim(id, name)` | Switches a sprite's animation (restarting it). |
| `prts.scene.model{ path, pos, yaw, scale, entity, name, solid }` | Loads a glTF 2.0 (`.glb`, `.gltf`) or MagicaVoxel (`.vox`) model. glTF keeps its origin and is scaled by `scale` (default 1); nodes named `marker_<x>` become markers `<name>.<x>`. Voxel models stand centred on `pos`, and `scale` is the size of one voxel (default 0.1 m). `solid = true` adds the bounding box as collision. Returns `ok, { min, max }`; a missing or broken model shows a glowing magenta box and `ok` is false. |

## `prts.audio`

Sound effects and music from `sounds/` and `music/` (WAV or OGG Vorbis). Buses: `"master"`,
`"effects"`, `"voice"`, `"music"`, `"ambience"`. Positional sounds fall off with distance, pan
with the commander's head, and are muffled automatically when every portal between their cell
and the commander's cell is closed. Audio never affects the simulation.

| Function | What it does |
|---|---|
| `prts.audio.play{ sound, at, entity, bus, volume, loop, range }` | Plays a sound at a point (`at`), on an entity (follows it, stops when it is gone) or without a position. `bus` defaults to `"effects"`. Returns a handle, or `nil` when every voice is busy. A missing file logs an error and plays a short placeholder beep. |
| `prts.audio.stop(h [, fade])` | Stops a sound, fading out over `fade` seconds. Returns `true` if it was playing. |
| `prts.audio.music{ track, fade, volume }` | Cross-fades to a streamed music track over `fade` seconds (default 1); `track = nil` fades the music out. |
| `prts.audio.set_volume(bus, v)` | Sets a bus volume (0..1). |
| `prts.audio.volume(bus)` | A bus volume. |
| `prts.audio.muffle(h, amount)` | Muffles a sound by script (0..1: quieter and duller). The walls' own muffling (`occlusion`) still applies; the stronger one is heard. Returns `true` if the sound is playing. |
| `prts.audio.voice(h)` | For tests and interfaces: `{ bus, volume, muffle, occlusion, fade, left, right, cutoff, loop, positional, stopping, placeholder }`, or `nil` if the sound is not playing. |
| `prts.audio.info()` | For tests: `{ device = "sdl" or "null", rate, voices, left, right, music }` (`left`/`right`: the level of the last mixed block). |

## `prts.entity`

| Function | What it does |
|---|---|
| `prts.entity.spawn{ name, parent, pos, yaw, usable }` | Creates an entity, optionally inside a parent frame and usable with the `use` action. Returns its handle. |
| `prts.entity.pos(e)` | World position `x, y, z` (resolved through every parent frame). |
| `prts.entity.local_pos(e)` | Position relative to the parent frame. |
| `prts.entity.yaw(e)` | Yaw relative to the parent frame. |
| `prts.entity.move_to(e, pos, speed)` | Moves the local position to `pos` at `speed` m/s; `move_done` fires on arrival. |
| `prts.entity.rotate_to(e, yaw, speed)` | Turns the local yaw to `yaw` at `speed` rad/s; `move_done` fires when done. |
| `prts.entity.moving(e)` | `true` while a move or turn is under way. |
| `prts.entity.set_solid(e, on)` | Enables or disables every collision box the entity owns. |
| `prts.entity.name(e)` | The entity's name (`""` if none). |
| `prts.entity.find(name)` | The first living entity with that name, or nothing. |
| `prts.entity.alive(e)` | `true` if the handle refers to a living entity (never raises). |
| `prts.entity.despawn(e)` | Destroys the entity and everything parented to it. |
| `prts.entity.data(e)` | The entity's script data: a table kept by the engine, created on first use. It is saved with the game, kept across hot reloads and part of the state hash, so it must hold **plain data** only (numbers, strings, booleans, tables, entity handles). Saving fails with the name of a field that holds a function, coroutine or userdata. Fetch it when needed rather than keeping the table across ticks: loading a save replaces it. |

## `prts.input`

Actions are defined by scripts (`data/base/scripts/input/defaults.lua`); the engine maps devices to
action values.

| Function | What it does |
|---|---|
| `prts.input.action{ id, keys, mouse_buttons, mouse_axes, pad_buttons, pad_axes }` | Defines an action or replaces its bindings. `keys` are key names (`"W"`, `"Left Shift"`); the other lists hold codes or `{ code = n, scale = s }`. |
| `prts.input.value(action)` | The action's current value (buttons 0 or 1, axes signed). |
| `prts.input.down(action)` | `true` while the action is held. |
| `prts.input.scheme()` | The active input scheme: `"kbm"`, `"gamepad"`, `"touch"` or `"xr"`. |

## `prts.commander`

| Function | What it does |
|---|---|
| `prts.commander.spawn{ marker \| pos, yaw, walk_speed, run_speed, sprint_speed, crouch_speed, jump_speed, eye_stand, eye_crouch, step_height, reach, look_sensitivity }` | Creates (or re-creates) the commander, the player's first-person body, at a marker or position. Returns its entity. |
| `prts.commander.entity()` | The commander's entity, or nothing before it is spawned. |
| `prts.commander.view()` | `yaw, pitch, eye_height, on_ground`. |

## `prts.render`

| Function | What it does |
|---|---|
| `prts.render.settings([t])` | Changes settings given in `t` (`preset` = `"authentic"`, `"retro"`, `"crisp"`, `"hd"` or `"native"`; booleans `dither`, `snap`, `affine`, `fog`, `crosshair`; `fov` in degrees) and returns the current ones (`lines`, `dither`, `snap`, `fog`, `renderer`). |

## `prts.save`

Save slots in the user data folder (`saves/<slot>.sav`). A save is a versioned binary snapshot of
the simulation (entities, frames, movers, collision switches, the commander, the random stream, the
tick and every entity's script data) on top of the booted map. Slot names use 1–64 letters, digits,
`_` or `-`.

| Function | What it does |
|---|---|
| `prts.save.write(slot)` | Saves now. Returns `true`, or `nil` and the reason (for example a field of entity data that is a function). |
| `prts.save.load(slot)` | Checks the save and loads it at the start of the next tick. Returns `true`, or `nil` and the reason. Refused while a replay is recorded or played. |

## Mod manifests (`mod.lua`)

Each mod folder has a `mod.lua` that returns a plain table (it runs without any globals):

```lua
return {
  id = "better_doors",              -- must match the folder name
  api = "1",                        -- prts.api_version the mod targets; other versions are not mounted
  version = "1.2.0",                -- optional, "major.minor.patch"
  depends = { "base >= 0.1", "keep_pack" },   -- optional: needs these mods (enabled with --mod)
  load_after = { "lighting_mod" },  -- optional: if that mod is enabled too, load after it
}
```

Dependencies accept `>=`, `>`, `=` (or `==`), `<=` and `<`. Mods load after the base game in a
fixed order: every mod after the mods it depends on or loads after, ties broken by id (the order
of `--mod` options does not matter). A mod whose dependency is missing, too old or skipped, or that
is part of a cycle, is not mounted, and the log says why.

## Hot reload (developer builds, desktop)

When you change a Lua module (a file loaded with `require`) while the game runs, it is re-run in
its mod's environment within about a second. If the module returns a table, the table already
known to `require` is updated in place, so other modules holding it see the new functions; event
handlers registered by name are replaced when the module registers them again. Entity data and the
rest of the world stay as they are. A module whose new version fails keeps its old version. After
a reload the engine calls `on_reload(names)` if `scripts/main.lua` defines it, with the list of
reloaded module names. `scripts/main.lua` itself is not re-run. Hot reload is on for interactive
runs of developer builds and can be turned on for tests with `--hot-reload`.

## Scenario tests (`t`)

A scenario file (run with `1stprts --headless --scenario <file>`) returns
`{ name = ..., max_seconds = ..., run = function(t) ... end }`. `run` is a coroutine that drives the
game through the same actions as real input; `t` offers:

| Function | What it does |
|---|---|
| `t.commander()` | The commander's entity. |
| `t.ticks([n])` | Waits `n` simulation ticks (default 1). |
| `t.wait_until(fn, seconds)` | Waits tick by tick until `fn()` returns true; fails the scenario after `seconds`. |
| `t.input.hold(action, seconds)` | Holds an action for that many simulated seconds. |
| `t.input.press(action)` | Presses an action for one tick. |
| `t.input.release(action)` | Ends a hold early. |
| `t.place(marker [, pitch])` / `t.place(x, y, z [, yaw [, pitch]])` | Teleports the commander (into the world frame). |
| `t.look(yaw [, pitch])` | Sets the view direction. |
| `t.face(entity_name [, ox, oy, oz])` / `t.face(x, y, z)` | Turns the view towards an entity (with an offset in its frame) or a point. |
| `t.pos(e)` | World position `x, y, z`. |
| `t.parent(e)` | Name of the parent frame, or `"world"`. |
| `t.in_zone(e, zone)` | `true` if the entity is inside a `prts.scene.zone`. |
| `t.in_cell(e, cell)` | `true` if the entity (1 m above its feet) is in that cell (the smallest containing cell). |
| `t.drawn_cells()` | Set of the cell names drawn in the last rendered frame (`{ ["keep.hall"] = true, ... }`); empty when headless. |
| `t.target()` | Name of the usable entity in view within reach, or nothing. |
| `t.hash()` | The 64-bit state hash as a hex string. |
| `t.tick()` | The current tick. |
| `t.headless()` | `true` without a window. |
| `t.screenshot(name)` | Saves `<name>.png` to the `--out` folder (windowed runs only). |
| `t.expect(value [, what])` | Returns checks that fail the scenario with a message: `.is_true()`, `.equals(x)`, `.near(x, tolerance)`. |

A scenario that only uses actions and look direction after its first wait can be recorded with
`--record <file>` and checked with `--replay <file>`; `tests/game/determinism/` holds such walks.

## Lua unit tests (`T`)

Files in `scripts/tests/` (run with `1stprts --headless --test-lua`) register cases on the global
`T`:

| Function | What it does |
|---|---|
| `T.case(name, fn)` | Registers a test case. |
| `T.eq(a, b [, what])` | Fails unless `a == b`. |
| `T.is_true(v [, what])` | Fails unless `v` is `true`. |
| `T.near(a, b, tolerance [, what])` | Fails unless `a` is within `tolerance` of `b`. |
| `T.fails(fn [, what])` | Fails unless calling `fn` raises an error. |

## Command line (for tests and CI)

`--test-lua`, `--scenario <file>`, `--junit <file>` (JUnit XML report of the Lua tests, the
scenario or the replay check), `--record <file>`, `--replay <file>` (exits with status 1 and names
the first tick whose state hash differs), `--hot-reload`, `--lua-coverage <file>`, `--seed <n>`,
`--ticks <n>`. Run `1stprts --help` for the full list.
