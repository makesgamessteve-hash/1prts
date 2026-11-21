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
| `prts.boot` | How this run was started: `{ auto_start, front_end, developer, headless, test, map }`. `auto_start` is true for headless, test, scenario and `--start` runs (unless `--front-end`), which begin a map at once instead of showing the front-end menu; `map` is the `--start` map; `developer` says whether `prts.dev` exists. |
| `prts.licenses()` | The third-party libraries linked into the executable, for the credits screen: array of `{ name, version, license, copyright, url }` (`version` may be `""`). |

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
| `prts.vfs.mounts()` | Array of `{ id = ... }` for every mounted pack (`"base"`, then the enabled mods) in load order; later mounts override earlier ones. |
| `prts.vfs.read_from(mount, path)` | A file of one pack only (by mount id), not the merged view: its contents, or `nil` and a reason (`"not found"`, also for an unknown mount; `"invalid path"`). The credits screen reads each pack's `CREDITS.md` this way. |

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
| `ui` | surface name, width, height | Every frame, once for each UI surface: the world surfaces (`prts.ui.surface`), then `"screen"`. Handlers draw with `prts.ui`. |
| `input_scheme` | scheme name | The active input scheme changed (see `prts.input.scheme`). |

The base game's scripts send events of their own the same way, for example `session_started`
(map id, options) from `game/session.lua` once a map is loaded; the options are
`{ prologue = true }` when the front-end menu asks for the map's prologue (unless the player
turned it off), and `{}` for test, batch and `--start` runs.

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
| `prts.scene.box{ min, max, material, entity, visible, solid, use_only, tile }` | An axis-aligned box: drawn unless `visible = false` (an invisible box needs no `material`), solid unless `solid = false`. `use_only = true` makes it a box only the `use` ray (and the XR pointer ray) hits, instead of a solid one: nothing collides with it, stands on it or is shadowed by it, so it suits people the commander talks to (`props/npc.lua` gives each person one). Returns its collision box id, or -1. |
| `prts.scene.wedge{ min, max, axis, sign, material, entity, tile }` | A ramp filling the box: its top rises from the box's floor to its top along `axis` (`"x"` or `"z"`) in the direction `sign` (1 or -1). Drawn only; add collision yourself (`lib/build.lua`'s `B.wedge` adds walkable invisible steps). |
| `prts.scene.tube{ base, radius, inner, height, sides, open, cap, material, entity, tile }` | A hollow round wall, such as a castle tower: `sides` flat sides (default 16), side `i` centred on the angle `i * 2π / sides` (0 is +x, then towards +z). Sides listed in `open` are left out with their edges closed (doors, windows, crenels); `cap` (default true) closes the top ring. Drawn only; `lib/castle.lua` adds collision. |
| `prts.scene.prism{ base, radius, top_radius, height, sides, material, entity, solid }` | A prism or cone (tree trunks, roofs, towers); solid only with `solid = true`. |
| `prts.scene.terrain{ size, spacing, seed, height, octaves, material, flats }` | Generates the terrain height field; `flats` is a list of `{ x, z, radius, height, blend }` areas levelled for buildings. |
| `prts.scene.height(x, z)` | Terrain height at a point. |
| `prts.scene.light{ pos, color, radius, entity }` | A point light (up to 8 nearest are used per view). With `entity`, `pos` is in that entity's frame and the light moves with it (a lantern in the lift cage). |
| `prts.scene.marker(name, pos [, yaw])` | A named position (spawn points, test markers). |
| `prts.scene.marker_pos(name)` | `x, y, z, yaw` of a marker, or nothing. |
| `prts.scene.zone(name, min, max)` | A named box used by tests and scripts to ask "is it inside?". |
| `prts.scene.in_zone(name, x, y, z)` | `true` if the point is inside the named zone; errors on an unknown zone. `lib/triggers.lua` uses it to send `zone_enter` and `zone_exit` events for watched entities. |
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
| `prts.scene.finish()` | Finishes building the map: hands the terrain to collision, moves the collision boxes of entity geometry to their entities' poses and, in windowed runs, bakes sunlight and shadows into the vertex lighting if the scene changed since the last bake. The engine calls it after `scripts/main.lua`; a map started later (from the menu) calls it itself. |

## `prts.audio`

Sound effects and music from `sounds/` and `music/` (WAV or OGG Vorbis). Buses: `"master"`,
`"effects"`, `"voice"`, `"music"`, `"ambience"`. Positional sounds fall off with distance, pan
with the commander's head, and are muffled automatically when every portal between their cell
and the commander's cell is closed. Audio never affects the simulation. In a browser nothing can
be heard before the player's first click, tap or key press; sounds and music started before then
are held and begin from their start at that moment, so scripts can start the menu music at boot.

| Function | What it does |
|---|---|
| `prts.audio.play{ sound, at, entity, bus, volume, loop, range }` | Plays a sound at a point (`at`), on an entity (follows it, stops when it is gone) or without a position. `bus` defaults to `"effects"`. Returns a handle, or `nil` when every voice is busy. A missing file logs an error and plays a short placeholder beep. |
| `prts.audio.stop(h [, fade])` | Stops a sound, fading out over `fade` seconds. Returns `true` if it was playing. |
| `prts.audio.music{ track, fade, volume }` | Cross-fades to a streamed music track over `fade` seconds (default 1); `track = nil` fades the music out. |
| `prts.audio.set_volume(bus, v)` | Sets a bus volume (0..1). |
| `prts.audio.volume(bus)` | A bus volume. |
| `prts.audio.muffle(h, amount)` | Muffles a sound by script (0..1: quieter and duller). The walls' own muffling (`occlusion`) still applies; the stronger one is heard. Returns `true` if the sound is playing. |
| `prts.audio.voice(h)` | For tests and interfaces: `{ bus, volume, muffle, occlusion, fade, left, right, cutoff, loop, positional, stopping, placeholder }`, or `nil` if the sound is not playing. |
| `prts.audio.info()` | For tests: `{ device = "sdl" or "null", rate, voices, left, right, held, mixed, music }` (`left`/`right`: the level of the last mixed block; `held`: `true` while the device does not play yet, as in a browser before the first click, tap or key press; `mixed`: seconds of audio mixed so far). |

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
action values. Touch axes 0 and 1 are the movement stick, 2 and 3 the look drag; touch button 0 is
a tap on the look side, 1 and up the context buttons of the touch layout. XR controllers use the
WebXR `xr-standard` order per hand, left hand first: `xr_buttons` 0-7 are the left controller's
(0 trigger, 1 squeeze, 2 touchpad, 3 thumbstick press, 4 X, 5 Y) and 8-15 the right one's (12 A,
13 B); `xr_axes` 0-3 are the left controller's (2, 3 thumbstick x and y, y is -1 pushed forward)
and 4-7 the right one's. A button reads 1 while the browser reports it pressed (at most 0.5
otherwise); axes have the gamepad's dead zone. In XR the head looks around, `look_x` turns (see
`prts.xr`), and `use` aims with the controller's pointer ray instead of the view. Players rebind
actions per scheme (`kbm`, `gamepad`, `touch`, `xr`); their overrides are kept in `prts.settings`.

| Function | What it does |
|---|---|
| `prts.input.action{ id, label, keys, mouse_buttons, mouse_axes, pad_buttons, pad_axes, touch_axes, touch_buttons, xr_buttons, xr_axes, always }` | Defines an action or replaces its default bindings. `keys` are key names (`"W"`, `"Left Shift"`); the other lists hold codes or `{ code = n, scale = s }`. `label` names it in the controls menu; `always = true` keeps it working while a menu or the console blocks game input. The player's saved override of a scheme replaces that scheme's defaults (an invalid one is ignored with a warning). |
| `prts.input.value(action)` | The action's current value (buttons 0 or 1, axes signed). |
| `prts.input.down(action)` | `true` while the action is held. |
| `prts.input.went_down(action)` | `true` if the action went down in the latest input update, once per frame (for menus, which run per frame rather than per tick). `false` for an unknown action. |
| `prts.input.scheme()` | The active input scheme: `"kbm"`, `"gamepad"`, `"touch"` or `"xr"`. |
| `prts.input.actions()` | Array of `{ id, label }` for every action in definition order (`label` is the id when the definition has none). |
| `prts.input.bindings(action)` | The action's current bindings by scheme: `{ kbm = { ... }, gamepad = { ... }, touch = { ... }, xr = { ... } }`, each a list of `{ kind, code, scale, name }`. `kind` is `"key"`, `"mouse_button"`, `"mouse_axis"`, `"pad_button"`, `"pad_axis"`, `"touch_axis"`, `"touch_button"`, `"xr_button"` or `"xr_axis"`; `name` (keys only) is the key name. Errors on an unknown action. |
| `prts.input.rebind(action, scheme, binding)` | Replaces the bindings of one scheme (`"kbm"`, `"gamepad"`, `"touch"` or `"xr"`) with `binding`: a table of lists like the definition's (`{ keys = { "F" } }`, `{ pad_axes = { { code = 2, scale = -1 } } }`, `{}` for none) or, for `kbm`, a key name. Stores it as the player's override (settings key `bindings.<scheme>.<action>`, written by `prts.settings.save`) and returns `true`. Errors on an unknown action or scheme, on lists of another scheme and on an invalid binding (the old bindings stay). |
| `prts.input.reset_bindings([scheme])` | Forgets the player's overrides of one scheme (or of all) and rebinds the defaults. |
| `prts.input.capture(scheme)` | For a controls menu: the first key or mouse button (`"kbm"`), gamepad button or axis pushed past 0.6 (`"gamepad"`), or XR controller button or axis pushed past 0.6 (`"xr"`), pressed since the last frame, as a binding table for `prts.input.rebind`; `nil` if none, and always for `"touch"`. |
| `prts.input.touch_layout{ split, stick_radius, look_speed, tap_distance, tap_seconds, buttons }` | Replaces the touch controls. A finger landing left of `split` (fraction of the width, default 0.5) drives the movement stick, fully deflected at `stick_radius` (fraction of the height, default 0.12); one on the right drags the look (`look_speed` look units per screen height, default 1000) or taps if it moves at most `tap_distance` (default 0.03 of the height) within `tap_seconds` (default 0.3). `buttons = { { id, x, y, radius }, ... }`: up to 15 round context buttons (position as fractions of the screen, radius of its height); button `i` drives touch button `i`, and all start hidden. Returns the number of buttons. |
| `prts.input.touch_visible(id, on)` | Shows or hides a context button; hidden buttons ignore fingers. Errors on an unknown id. |
| `prts.input.touch_state()` | For drawing the overlay (the engine draws none itself): `{ stick = { ox, oy, kx, ky }, buttons = { { id, x, y, radius, visible, held, touch_button }, ... } }`. `stick` (the stick's origin and knob, fractions of the screen) is there only while a finger holds it; `touch_button` is the code to bind with `touch_buttons`. |

## `prts.commander`

| Function | What it does |
|---|---|
| `prts.commander.spawn{ marker \| pos, yaw, walk_speed, run_speed, sprint_speed, crouch_speed, jump_speed, eye_stand, eye_crouch, step_height, reach, look_sensitivity }` | Creates (or re-creates) the commander, the player's first-person body, at a marker or position. Returns its entity. |
| `prts.commander.entity()` | The commander's entity, or nothing before it is spawned. |
| `prts.commander.view()` | `yaw, pitch, eye_height, on_ground`. |
| `prts.commander.target()` | `entity, name` of the usable entity in view within reach (in XR: on the pointer ray of the pointer hand), or nothing (also before the commander is spawned). The ray stops at the first box it hits, solid or `use_only`, and the entity owning that box counts if it is usable. |

## `prts.xr`

Immersive VR (WebXR in the web build). During a session the tracked head turns the view and the
body follows the head across the play area; walls stop the body, and the view stops 12 cm short of
any surface the head leans into. Each controller shows a pointer ray; the hand whose trigger was
pulled last is the pointer hand, which `use` (and in-world surfaces such as the war table) aims
with. Head and hand poses reach the simulation as per-tick input, recorded in replays like the
look direction. `game/settings.lua` keeps the settings below under `xr.*` in `prts.settings` and
applies them at boot; the options menu has a VR page.

| Function | What it does |
|---|---|
| `prts.xr.settings([t])` | Changes the comfort options and video settings in VR given in `t` and returns all of them. Comfort: `locomotion` (`"smooth"`: the left stick walks where the head looks; `"teleport"`: pushing the stick forward aims with that hand's ray, letting go goes to the floor it points at, if there is room to stand and no wall in between), `turn` (`"snap"` or `"smooth"`: how `look_x` turns), `snap_angle` (degrees, 5-180, default 45), `turn_speed` (degrees per second, 10-720, default 120), `vignette` (the comfort vignette while walking or turning smoothly, default on), `vignette_strength` (0-1, default 0.6), `seated` (raises the view to standing eye height, measured from the head when turned on or when a session starts). Video in VR (the flat screen's own settings stay as they are; fog follows the flat screen): `preset`, `dither`, `snap`, `affine`, `crosshair`, defaults `"hd"` and all off (spec *Comfort defaults*). Errors on an unknown name or a value of the wrong type. |
| `prts.xr.state()` | `{ supported, active, tracking, sessions, views, left, right, pointer, turn, seat, vignette, aiming, pushback, origin }`: whether the browser offers VR, a session runs and the head is tracked; sessions so far; eye views of the last frame; which controllers are there; the pointer hand (`"left"` or `"right"`); the play area's yaw (radians); the seated height added; the vignette's current strength; whether a teleport is being aimed; how far the last view was pushed out of a wall (meters); and `origin`, the world position `{ x, y, z }` of the play area's origin. |
| `prts.xr.exit()` | Ends the session (the game's menu: menus are drawn on the flat page, so the menu button leaves VR). Returns whether one was running. |

## `prts.render`

| Function | What it does |
|---|---|
| `prts.render.settings([t])` | Changes settings given in `t` (`preset` = `"authentic"`, `"retro"`, `"crisp"`, `"hd"` or `"native"`; booleans `dither`, `snap`, `affine`, `fog`, `crosshair`; `fov` in degrees) and returns the current ones (`lines`, `dither`, `snap`, `fog`, `renderer`). |

## `prts.ui`

An immediate-mode UI: scripts draw it again every frame from a `ui` event handler,
`prts.events.on("ui", name, function(surface, w, h) ... end)`, which runs once per frame for each
surface: `"screen"` (the overlay over the view) or an in-world surface (`prts.ui.surface`).
Scripts never see microui; they get windows, rows, widgets and direct drawing. Positions and sizes
are pixels of the surface; the screen is the window size divided by a whole-number scale, so it is
about 360 pixels high. Colors are `{ r, g, b [, a] }` from 0 to 255 (default white). Drawing
outside a `ui` handler, a widget outside a window, a text over 1024 bytes (8192 for
`prts.ui.text`) and too much UI in one frame are errors; an error inside a window's or panel's
`fn` still closes it.

In screen space the First-Person Contract allows only a minimal HUD (`ui/hud.lua`) and the touch
overlay; menus (front-end, pause, options, credits) are allowed. Displays the commander reads
during play belong on surfaces in the world.

| Function | What it does |
|---|---|
| `prts.ui.window(title, x, y, w, h [, opts], fn)` | A window at `x, y` of size `w, h` (at least 16): calls `fn` to fill it, then closes it. The title also identifies the window. `opts` holds booleans, all default true: `title` (title bar), `frame`, `scroll` (scroll bars) and `input` (takes the pointer). Windows stay where the script puts them; the player cannot move, resize or close them. At most 24 windows and 40 windows and panels per frame, nested at most 12 deep. |
| `prts.ui.panel(name, fn [, scroll_to_end])` | A scrollable area filling the next layout cell, filled by `fn`. `scroll_to_end = true` scrolls it to the bottom (for logs). |
| `prts.ui.row(widths [, height])` | Lays out the following widgets in rows of 1–16 cells with these widths; widgets fill the cells left to right and wrap into another row of the same widths. `0` (and the default height) is the standard size; a negative value `-n` stretches the cell to `n - 1` pixels before the right (or bottom) edge, so `-1` fills the rest. |
| `prts.ui.label(text [, centered])` | One line of text in the next cell, centered if `centered` is true. |
| `prts.ui.text(text)` | A paragraph in the next cell, wrapped to its width (up to 8192 bytes). |
| `prts.ui.button(label [, id])` | A button; returns `true` in the frame it is clicked. `id` (default: the label) keeps its identity when the label changes; tests can click it by either. |
| `prts.ui.checkbox(label, value [, id])` | A checkbox showing `value`; returns the new value and whether it changed this frame. The script keeps the value and passes it in every frame. |
| `prts.ui.slider(id, value, low, high [, step])` | A horizontal slider showing its value (whole numbers when `step >= 1`, else two decimals). Returns the new value, clamped to `low..high` and rounded to `step` while dragged (`0`, the default: no rounding), and whether it changed. Needs `low < high` and `step >= 0`. |
| `prts.ui.textbox(id [, text [, focus]])` | A one-line text field showing `text` (default `""`, at most 255 bytes); returns the edited text and `true` in the frame Return is pressed. `focus = true` gives it the keyboard focus. |
| `prts.ui.draw_text(text, x, y [, { align, scale, color }])` | Draws text over the widgets, also outside windows. `align` is `"left"` (default), `"center"` or `"right"` of `x`; `scale` is a whole number from 1 to 8 (default 1). |
| `prts.ui.draw_rect(x, y, w, h [, color])` | A filled rectangle over the widgets. |
| `prts.ui.draw_icon(name, x, y, w, h [, color])` | An icon of the UI font stretched over the rectangle: `"white"`, `"check"`, `"close"`, `"collapsed"`, `"expanded"`, `"disc"` or `"ring"`. Errors on any other name. |
| `prts.ui.text_width(text)` | Width of `text` in pixels at scale 1. Works outside a frame too. |
| `prts.ui.font()` | `{ height, line_height }` of the UI font (`fonts/ui.lua`). |
| `prts.ui.size()` | `w, h` of the surface being drawn, or of the screen outside a frame. |
| `prts.ui.set_modal(on)` | Menu mode: while on, the pointer belongs to the menu, game actions read as released (except actions defined with `always = true`, such as `menu`) and world surfaces ignore the commander's view. |
| `prts.ui.modal()` | `true` while menu mode is on. |
| `prts.ui.hovered()` | `true` if the pointer was over a window on the screen in the last frame. |
| `prts.ui.surface{ id, width, height, origin, right, down }` | Adds an in-world UI surface, or resizes the one with that `id`, and returns its slot. `width` and `height` are 8–1024 pixels (default 256); at most 7 surfaces; the id `"screen"` is taken. `origin` is the world position of the top-left corner, `right` and `down` the vectors along the top and left edges (meters). Within 4 m the commander's view (or a finger) points at it, and `use` or the left mouse button clicks. `ui` handlers draw it when `surface` is its id. A material with `surface = <id>` shows it on an object (see `props/notice_board.lua`). |

## `prts.settings`

The player's settings: one table of plain data, read from `config.lua` in the user data folder
at start (before any script runs) and written back only by `prts.settings.save`. Keys are dotted
paths of 1–8 names, each shorter than 64 bytes: `"video.fov"` is `settings.video.fov`. Setting a
value applies nothing by itself; game scripts read the settings (`game/settings.lua` applies the
video and audio ones). Binding overrides live under `bindings` (see `prts.input.rebind`).

| Function | What it does |
|---|---|
| `prts.settings.get(key [, default])` | A copy of the value, or `default` if there is none. |
| `prts.settings.set(key, value)` | Sets a value, creating tables along the path; `nil` removes it. Values are plain data (booleans, finite numbers, strings, and tables of them with string or integer keys); anything else is an error. |
| `prts.settings.save()` | Writes every setting to `config.lua`, keys sorted so the file stays stable. Returns `true`, or `nil` and the reason. |

## `prts.save`

Save slots in the user data folder (`saves/<slot>.sav`). A save is a versioned binary snapshot of
the simulation (entities, frames, movers, collision switches, the commander, the random stream, the
tick and every entity's script data) on top of the booted map. Slot names use 1–64 letters, digits,
`_` or `-`.

| Function | What it does |
|---|---|
| `prts.save.write(slot)` | Saves now. Returns `true`, or `nil` and the reason (for example a field of entity data that is a function). |
| `prts.save.load(slot)` | Checks the save and loads it at the start of the next tick. Returns `true`, or `nil` and the reason. Refused while a replay is recorded or played. |

## `prts.app` and `prts.dev`

`prts.dev` exists only in developer builds, and not with `--no-dev` (`prts.boot.developer` says
which). Release builds have no developer console and no observer camera (First-Person Contract).

| Function | What it does |
|---|---|
| `prts.app.quit()` | Stops the frame loop after the current frame (the menus' Quit button). Lua unit tests, which do not run the frame loop, carry on. |
| `prts.dev.console([open])` | Opens or closes the developer console (the backquote key does the same) and returns whether it is open. |
| `prts.dev.observer([on])` | Turns the observer camera on or off and returns whether it is on: a free camera labeled "DEVELOPER VIEW" on screen, moved with the mouse, W/A/S/D, Space, Left Ctrl and Left Shift, while game input is blocked. |
| `prts.dev.run(line)` | Runs a console command as if it was typed: `help`, `clear`, `reload` (hot reload, where the build has it), `observer` (toggles the camera), or any other line as Lua in the shared script environment (the values of an expression are printed to the console). |

## Mod manifests (`mod.lua`)

Each mod folder has a `mod.lua` that returns a plain table (it runs without any globals):

```lua
return {
  id = "better_doors",              -- must match the folder name
  api = "1",                        -- prts.api_version the mod targets; other versions are not mounted
  version = "1.2.0",                -- optional, "major.minor.patch"
  title = "My Game",                -- optional, base game only: the window title (default "1stPRTS")
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
`{ name = ..., max_seconds = ..., final_cell = ..., run = function(t) ... end }`. `run` is a
coroutine that drives the game through the same actions as real input. When it ends, the runner
reports the result with the commander's final cell and the state hash
(`scenario PASSED (410 ticks, final cell 'keep.floor3', state hash ...)`); a scenario that names a
`final_cell` fails when the commander ends in any other cell. A `-- ctest-labels: smoke` line puts
a scenario into the commander-is-there smoke set, which also runs in the web build. `t` offers:

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
| `t.ui_has(text [, surface])` | `true` if a text drawn in the last frame on the surface (default the screen) contains `text`: labels, paragraphs, window titles, button and checkbox labels and `prts.ui.draw_text`, also when scrolled out of view. |
| `t.ui_texts([surface])` | Array of those texts. Errors on an unknown surface. |
| `t.ui_click(label)` | Clicks the screen widget (button, checkbox, slider or text box) whose label or, failing that, id is `label`, at its centre, over a few ticks. Fails if no such widget was drawn in the last frame. |
| `t.key(name)` | Presses and releases a key by name (`"Escape"`, `"Return"`, `` "`" ``), one tick each; the keyboard becomes the last used device. Errors on an unknown key name. |
| `t.type_text(s)` | Types text into the focused text field or the console, 16 bytes per tick. |
| `t.scheme(name)` | Switches the active input scheme (`"kbm"`, `"gamepad"`, `"touch"` or `"xr"`). |
| `t.pad_button(code, down)` / `t.pad_axis(code, value)` | A connected gamepad: presses or releases button `code` (0–31), or sets axis `code` (0–5). |
| `t.touch(slot, x, y, down)` | Puts finger `slot` (1–10) down at `x, y` (fractions of the window), or lifts it. |
| `t.console_open()` | `true` while the developer console is open. |
| `t.console_lines()` | The developer console's lines (empty without a console). |
| `t.observer()` | `true` while the observer camera is on. |
| `t.camera()` | `x, y, z, yaw` of the view being rendered (the observer camera when it is on). |
| `t.xr_begin([width, height])` | A simulated headset starts an XR session (framebuffer 2000 x 1000 by default). |
| `t.xr_frame{ head, left, right, lost }` | The poses and controllers of the following XR frames, packed as the web glue packs them, until the next call. Poses are `{ pos = { x, y, z }, yaw, pitch }`, `{ pos, dir = { x, y, z } }` or `{ pos, quat = { x, y, z, w } }` in the play area (meters, y up, -z ahead; default head 1.6 m up, looking ahead). A hand is `{ aim = pose, grip = pose, trigger, squeeze, buttons = { b0, b1, ... }, stick = { x, y }, axes = { a0, a1, a2, a3 } }` (buttons count as pressed from 0.5); a missing hand is not connected. `lost = true`: tracking lost (no head, no views). Errors without a session. |
| `t.xr_end()` | The headset ends the session. |
| `t.xr_view()` | `x, y, z` of the head's view in the world as it is drawn (after the wall push-back), or nothing outside a session. |

The UI and device helpers, `t.ui_has` to `t.xr_view`, also exist on `T` in Lua unit tests, where
each step runs one UI frame at once instead of waiting a tick.

A scenario that only uses actions, look direction and the simulated headset after its first wait can be recorded with
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
| `T.ui_frame([n])` | Runs `n` UI frames at once (default 1): input, every `ui` handler and the drawing records `T.ui_has` reads, without simulation ticks. |
| `T.ui_has`, `T.ui_click`, `T.key`, ... | The UI and device helpers of the scenario object `t` (see above). |

## Command line (for tests and CI)

`--test-lua`, `--scenario <file>`, `--junit <file>` (JUnit XML report of the Lua tests, the
scenario or the replay check), `--record <file>`, `--replay <file>` (exits with status 1 and names
the first tick whose state hash differs), `--hot-reload`, `--lua-coverage <file>`, `--seed <n>`,
`--ticks <n>`. Run `1stprts --help` for the full list.
