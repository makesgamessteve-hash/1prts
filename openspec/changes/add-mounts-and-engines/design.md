# Design: mounts and engines

## The commander on horseback (`props/horse.lua`)

- A horse is an entity with a horse sprite and a `use_only` box. Using it (or the `mount`
  action within 3 m) mounts it: the horse entity is despawned and its id kept in the commander's
  mount state (an entity named `commander.mount` holds the data). The commander's
  parameters are changed with `prts.commander.tune`:
  - walk 3 m/s (trot), run 7.5 m/s (canter), sprint 12 m/s (gallop);
  - crouch speed 0;
  - eye 2.55 m;
  - step 0.5 m.

  The previous parameters are kept in the mount state.
- While mounted, a child entity of the commander carries the horse's neck, head and ears (boxes)
  ahead of and below the eye, so the rider sees the horse (First-Person Contract).
- `mount` again (or use with nothing in view) dismounts: the parameters come back and a horse is
  spawned beside the commander.

## Cavalry (`rts/defs.lua`)

- **Stables** (Crown, 18 × 12 m) train **knights** (squad 4, hp 140, armour 3, speed 4.8 m/s,
  lance 14 damage) and outriders.
- **Charge:** a mounted unit's first strike after moving at more than half its speed does double
  damage (`charge = 2`).
- **Ride with me:** the order `follow` sets `order.follow = commander`. Companies with it re-path
  to a slot behind the commander every second. It is on the stone (`Ride with me`) and on the
  horn (the fourth signal, `rally`, stays as it is; a fifth signal `follow` is added).

## Turning with the frame (engine)

`prts_commander_tick` adds the parent frame's yaw change of this tick
(`yaw − yaw_interp(0)`) to the view yaw (not in XR, where the head is the view). Movers run
before the commander, so the change is this tick's. No new saved state: the world saves the
previous yaw.

## Posts (`lib/posts.lua`)

- A post is a usable entity with `kind = "post"`, a `post` name and an anchor (local position
  and facing in its frame). Taking it:
  - moves the commander to the anchor;
  - sets walk, run and sprint to 0 with `tune`, so the move actions are free for the post;
  - stores the post in the commander's state.
- Each tick the post's handler reads the move actions (`move_forward` / `move_back`, `move_left` /
  `move_right`) and drives its machine. `use` on the post again, or `jump`, leaves it.
- Posts in M3:
  - the wagon's driver's seat: forward and back set the speed (0 to 4 m/s), left and right turn;
  - the siege tower's push bar: a word to the crew, not a seat. Using it sets them pushing at
    0.8 m/s towards the wall (or stops them), and the commander is free at once to climb while
    the tower rolls;
  - the bridge lever on the tower's top floor: lowers the bridge onto the wall.

## Vehicles (`props/wagon.lua`, `props/siege_tower.lua`)

- A vehicle is an entity whose boxes are its own. It moves with `prts.entity.move_to` and
  `rotate_to` in small steps each tick, so whoever stands on its deck rides along (the engine
  parents the commander to the frame they stand on).
- **The wagon** follows a road (a list of points) when no one drives it, at 2.5 m/s, turning
  towards the next point.
- **The siege tower** is a deck plan of three levels:
  - the ground level has the push bar and stairs up;
  - the middle level has more stairs;
  - the top level has a bridge.

  The bridge is a child entity that slides forward 3 m onto the wall top when lowered.

## Deck plans (`lib/deckplan.lua`)

```lua
deckplan.compile{ entity = e, cell = 1.0, origin = { x, y, z }, material = "planks", wall = "oak",
  levels = {
    { y = 0, height = 3.2, rows = {
        "#####",
        "#s..#",   -- s: stairs up (rising towards +z), . floor, # wall, D doorway, W window
        "#s.r#",   -- letters a-r: rooms (zones named <prefix>.<room>), the floor under them
        "##D##",
    } },
  },
  rooms = { r = "push" }, prefix = "tower" }
```

- Each level: floors under every non-blank cell; walls of `#` (full height), `W` (window: wall
  below 1 m and above 2.2 m), `D` (doorway: lintel above 2.4 m). Runs of the same tile on a row
  are merged into one box.
- Stairs: a run of `s` cells in a column forms a stair rising one level over its length.
  It is made of steps of at most 0.3 m, and the floor above is left open over the run.
- Rooms: every cell with a room letter belongs to that room; its bounds over all its cells are
  a zone `<prefix>.<name>` in the frame (via `prts.scene.zone` for static plans; for moving
  frames the compiler returns local bounds and `deckplan.room_at(plan, e, x, y, z)` answers in
  the frame).
- Returns `{ rooms = { name = { min, max, level } }, levels, boxes }`.

## Crews (`lib/crew.lua`)

- `crew.new{ frame = e, name, people = { { name, room, kind, slot = { x, y, z } } } }` stores a
  roster in the frame's entity data.
- Level of detail by the commander's distance to the frame:
  - embodied within 45 m (a sprite per person, parented to the frame at its slot);
  - rostered within 250 m (no sprites; names and rooms kept);
  - aggregated beyond (`crew.count`).

  Hysteresis of 5 m. Checked every 15 ticks. `crew.lod(e)` reports the level.
- `crew.hurt_room(e, room, n)` removes up to `n` people in that room (fewer embodied sprites).

## Room damage (`lib/crew.lua`, `props/siege_tower.lua`)

- Rooms have `hp`. `rooms.damage(e, room, amount)` lowers it. Each 25% lost costs the room one
  of its crew. At 0 the room is wrecked: a dark scorched floor box is added in the room and its
  post stops working. The fort's towers shoot the siege tower's rooms when it is in range.

## The proving map: Greyfen March (`maps/greyfen_march.lua`)

- About 600 m × 400 m of meadow:
  - the Crown's camp in the west: stables with three horses, a wagon on a loop road, a
    company of knights;
  - the rival fort in the east: a curtain wall 6 m high with two towers, built with `lib/castle.lua`;
  - the siege tower 60 m in front of the fort's west wall.
- A rock hill in the north-west holds the gold mine. The adit at ground level opens into a
  gallery (deck plan) with a lift shaft. The lift cage (`props/lift.lua`) rides 8 m up to an
  upper gallery with a gold vein and a lookout over the march.
- In the menu as "Greyfen March (Ride Out)".
