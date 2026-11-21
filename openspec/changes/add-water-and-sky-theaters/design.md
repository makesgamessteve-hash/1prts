# Design: water and sky

## Water (`rts/water.lua`)

- `Wa.setup{ level, area = { x0, z0, x1, z1 } }`: the water surface is a non-solid box at
  `level`. A hidden solid floor at `level - 1.1` lets the commander wade.
- `Wa.depth(x, z)` is `level - ground`.
- While the commander wades (depth > 0.4 under the feet), their speeds drop to half. The tick
  restores them on dry land.
- Engine: `prts_nav_block_below(n, height, delta)` adds `delta` to the block counter of every
  cell whose ground is below `height`; `prts.nav.block_below(height)` and `unblock_below`. Land
  units keep to land.

## Ships (`props/ship.lua`, `rts/fleet.lua`)

- **Definitions:** `D.SHIPS`, with fields:
  - `id`, `name`, `faction`, `length`, `beam`, `draft`;
  - `speed`, `turn`, `hp`;
  - `guns = { damage, range, cooldown, per_side }` or `ram = damage`;
  - `cargo` (passengers), `crew` (rostered people), `plan` (deck-plan levels and rooms);
  - `cost`, `at` (harbor or raft-yard), `tier`.
- **A ship** is an entity at the waterline:
  - its hull is boxes (sides, bow and stern) around a deck plan;
  - a ship's rooms have health (`lib/crew.lua`);
  - the ship's data holds `side`, `def`, `hp`, `flood` (0..1), `dest`, `cargo` (company records)
    and `reload`.
- **Moving:**
  - Each tick the ship turns towards `dest` at its turn rate and moves at its speed, slower as it
    floods.
  - It stops if the water ahead (bow + 6 m) is shallower than its draft.
  - It is placed with `prts.entity.place` at `level - draft * flood_sink`. The commander on deck
    rides along (M3 frames).
- **Fighting:**
  - Every second, a ship with guns finds the nearest enemy ship or structure within range
    (structures by footprint distance) and fires the guns of that side when reloaded.
  - Each hit lands on a room chosen in turn: damage to the room, and to the ship's `hp`.
  - Presence of the commander aboard shortens the reload by a quarter.
  - A ram galley under way that passes within 4 m of an enemy hull deals `ram` damage once per
    pass, to the hull rooms.
- **Damage control:**
  - rooms flagged `hull` (below the waterline) let in water while damaged: `flood += 0.004 per
    second per damaged hull room`;
  - living crew in the `pumps` room bail: `flood -= 0.0006 per second per person`;
  - at `flood >= 1` or `hp <= 0` the ship sinks: it settles under the surface over 6 s, its crew
    and cargo are lost, and an event fires.
- **Transport:**
  - `Fl.embark(ship, company)`: members within 25 m are taken aboard (their units go, their
    records are kept in `cargo`).
  - `Fl.land(ship)`: the companies come ashore at the nearest dry ground within 30 m of the bow,
    with their records.
- **The chart room:** the galleon's chart room holds a war table (`rts/tools.lua` furnishes any
  host with a base point). It is a naval command post.

## Harbors

- `crown.harbor` (T3, 40 × 24) and `wildborn.raft_yard` (T3, 36 × 22) must stand with their
  seaward side (+z) within 10 m of water at least 3 m deep.
- They train ships: a ship is launched 18 m off the seaward side.

## The sky (`rts/sky.lua`, `props/balloon.lua`, `props/skyship.lua`)

- **The observation balloon:**
  - an envelope on a basket, tethered to a winch post;
  - the winch is a lift: use raises or lowers the basket between 2 m and 40 m. The commander
    in the basket rides up.
  - While up, it is an eye for its side with sight 320 m (picture).
  - It can be shot down (hp 300; only missiles and flyers reach it).
- **Wyvern riders** (`wildborn.wyvern`, at roost cliffs, tier 3):
  - flying units, not agents: their entity is moved by `rts/sky.lua` at 18 m above the ground
    or water, at 9 m/s, towards their order or target;
  - they attack ground and air from above (dive within 4 m);
  - only missile weapons and towers reach them;
  - their sprite is a wyvern with its rider.
- **The dwarven skyship:**
  - a deck-plan gondola under an envelope, flying at 45 m with a turn rate like a ship's;
  - it moves to its destination; the commander rides on deck;
  - it is boarded from a skyship mast's lift.

## Saltcoast (`maps/saltcoast.lua`)

- A bay opening east. The Crown's harbor town (keep, harbor, houses, two watchtowers) is on the
  north shore; the Wildborn siege camp (war lodge, raft-yard, war pits, totem watches,
  wyverns, longships and ram galleys) is on the south shore.
- The relief fleet is at the bay's mouth: the galleon (player), the caravel and two cogs with
  four companies. A skyship drifts over the hills to the west.
- Side 1 (Crown) is the player; side 2 (Wildborn) is an AI with a fleet plan: its ships attack
  ships in the bay and wyverns hunt.
- **Victory:** the war lodge falls. **Defeat:** the keep falls or the galleon sinks.
