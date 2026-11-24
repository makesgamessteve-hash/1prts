# Design: The map and scenario editor (M10)

## `prts.files` (engine, `api_files.c`)

- **The folder.** Files live in `<user>/files/<name>`.
- **Names.** 1–64 letters, digits, `_` or `-`, with an optional `.ext` of 1–8 lowercase letters
  or digits. Any other name (a path) is an error.
- **The calls:**
  - `write(name, text)`: at most 1 MB;
  - `read(name)`: text, or `nil` and why;
  - `list()`: sorted names, skipping files with other names;
  - `remove(name)`.
- **On the web** the user folder is IndexedDB-backed, so maps are kept there too.

## The map format (`game/mapfile.lua`)

- **The lines** (see the module's header):

  ```
  format 1
  name Keep Test
  size 1024 / seed 42 / hills 12 / time day|dusk|night
  start x z yaw
  place <kind> <id> x z [yaw [side]]
  ```

- **Reading.** `Mf.parse` checks every line:
  - number ranges;
  - known kinds, props and holdfasts;
  - a side for holdfasts and companies.

  It returns `nil, "line N: why"` on the first error. The text is never given to `load`, so a
  map file cannot run code.
- **Writing.** `Mf.write` writes a map back in a stable form, so reading and writing round-trips.
- **The sides.** `Mf.sides(map)` gives one holdfast per side, in side order.

## Building (`game/mapbuild.lua`)

- **`Mb.ground`:**
  - materials;
  - the terrain, flattened round holdfasts;
  - light and sky for the time of day (day, dusk, night).
- **`Mb.stand_in(o, name)`** is the editor's look of one object. Each is its own entity, so it
  can be removed and turned:
  - trees, mines and springs as their shapes;
  - ruins as their ring of stones;
  - props as their models;
  - a holdfast as a banner pole and footprint;
  - a company as one soldier.
- **`Mb.play`** builds the real resources and props.
- **`Mb.match`** (after the navigation grid):
  - starts a match when there are two or more holdfasts, with side 1 the player's;
  - spawns each company (six of its unit, or men-at-arms for an unknown id);
  - lets the Custodians (neutral) take the ruins.

## The editor (`ui/editor.lua`, `maps/editor.lua`)

- **The palette.** Trees, mines, springs, ruins, five props, four holdfasts (sides 1 to 4), two
  companies and the commander's start.
- **Actions** (new input actions, rebindable):

  | Action | Bindings |
  |---|---|
  | `editor_place` | left mouse, P |
  | `editor_remove` | Delete, Backspace |
  | `editor_next` / `editor_prev` | `]` / `[`, PageDown / PageUp |
  | `editor_turn` | T |

  They act only while the editor's map is loaded and no menu is open.
- **Where things go.** The ground in view (`Cmd.look_point`).
  - Placing a holdfast for a side that has one moves it.
  - Removing or turning picks the nearest object within 6 m of the point in view.
- **The readout** is drawn at the top of the screen. The First-Person Contract holds: the
  commander walks the map, and there is no free camera.
- **The game menu** in the editor:
  - Save map;
  - Play this map: saves, then `prts.app.restart("custom")`;
  - Map settings: size, hills, time, new ground. Each saves and restarts the editor so the
    ground is rebuilt.
- **Which file.** The setting `editor.file` names it, for the editor and for `maps/custom.lua`.
