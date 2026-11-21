# Design: the RTS core

## Navigation grid (`engine/src/nav/nav.c`, `prts_nav`)

- A rectangle of the XZ plane in square cells (default 2 m). A cell is walkable if the ground
  under its centre (terrain or the top of a solid box) is reachable without a step over the
  commander's step height from its neighbours, the slope is within the limit, and a body of
  0.6 m × 1.8 m fits (no solid box in the clearance column). Cells store their ground height.
- `prts_nav_block(rect, on)` marks cells blocked by buildings (a counter, so overlaps undo
  cleanly).
- A* over 8 neighbours (no diagonal corner cutting), octile heuristic, binary heap with ties
  broken by cell index: same inputs, same path. Paths are smoothed by walking the grid line
  between points (Bresenham) and dropping points that have a clear line. A start or goal in a
  blocked cell snaps to the nearest walkable cell (spiral search).
- Built by `prts.nav.build{ min, max, cell }` after the map is finished. Bounded search: at most
  N expanded nodes (default 200 000); no path → `nil`.

## Agents (`engine/src/sim/agents.c`)

- A table of agents keyed by entity, iterated in entity order (deterministic).
- Per tick: advance along the path at `speed`, steering towards the next point; separation from
  agents within 2 radii (a uniform spatial hash rebuilt each tick, neighbours visited in entity
  order); the agent's y follows the nav grid's ground height; yaw turns towards motion. On the
  last point an `agent_arrived` event fires (queued, dispatched with the tick's events).
- Agents are part of the state hash and saves (snapshot format 3): position comes from the
  entity, the agent stores speed, radius, path and index.
- Queries: `prts.agent.near(x, z, r [, max])` returns agent entities sorted by distance, ties by
  handle.

## Lua API

```
prts.nav.build{ min = {x, z}, max = {x, z}, cell = 2 }
prts.nav.path(x0, z0, x1, z1)        -> { {x, z}, ... } | nil
prts.nav.walkable(x, z)              -> bool
prts.nav.height(x, z)                -> ground height | nil
prts.nav.block{ min = {x, z}, max = {x, z} }  /  prts.nav.unblock{ ... }
prts.agent.add(e, { speed, radius })
prts.agent.move(e, x, z)             -> true if a path was found
prts.agent.follow(e, { {x, z}, ... })
prts.agent.stop(e)  prts.agent.remove(e)
prts.agent.info(e)                   -> { moving, speed, radius, tx, tz } | nil
prts.agent.set_speed(e, s)
prts.agent.near(x, z, r [, max])     -> { e, ... }
event "agent_arrived" (e)
```

## Rules in Lua (`data/base/scripts/rts/`)

| Module | Job |
|---|---|
| `defs.lua` | Crown tier 1 units and structures (`prts.def.unit`, `prts.def.structure`) |
| `world.lua` | Sides, resources, entities by side, spawning units and structures |
| `economy.lua` | Mines and trees, gathering trips, deposits, food and folk |
| `build.lua` | Placing sites, construction, structures as entity-owned geometry, nav blocking |
| `train.lua` | Training queues |
| `combat.lua` | Throttled think (every 6 ticks, staggered by handle): targets, attacks, damage, morale, death |
| `companies.lua` | Companies (squads), names, formations, standing orders |
| `orders.lua` | Orders and the sending-stone network (hubs, range, delay) |
| `picture.lua` | Sightings per side: seen, old; ages; what the war table and field map show |
| `ai.lua` | The opposing commander: economy, build order, army, attack waves, defence |
| `match.lua` | Starting a match, victory and defeat |

Determinism: all iteration uses `prts.sorted_pairs` or arrays in creation order; randomness comes
from `prts.rand` streams per side.

## Command tools (`data/base/scripts/command/`)

- **War table**: a UI surface on the war table in the holdfast's war room, drawn from the side's
  picture: terrain as golden contour dots, own companies and structures, sightings with their
  state, the stone network. The pointer (mouse, touch, XR ray) selects companies and gives
  orders; side panels show works (build, train) and dispatches.
- **Field map** (Tab): the same picture, smaller, held in front of the eyes; the commander walks
  slowly while it is open.
- **Sending stone** (hold Q): radial menu, ring 1 who (all, companies), ring 2 what (hold,
  advance to my mark, fall back, rally on me, report).
- **Spyglass** (hold right mouse): zoom; **marking staff** (left click while the spyglass or
  staff is raised): marks a point; with a company selected, it is ordered there; in build mode
  it places the building outline.
- **Horn** (H, then 1-4): advance, hold, fall back, rally, to companies within 2 km.

## Testable headless

Navigation and agents are C unit tests; rules are Lua unit tests on small set-ups; scenarios
play gathering, training, a fight, orders over the network, and a whole AI-versus-AI match to
the end (deterministic, CI on every platform).
