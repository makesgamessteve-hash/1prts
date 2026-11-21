# Simulation

## 1. Fixed tick

- The simulation advances in **fixed ticks of 1/30 s**, independent of frame rate. If a frame
  falls behind, at most a set number of ticks (default 5) are caught up. After that the game
  slows down instead of spiraling.
- Time inside the simulation is the **tick counter**. Scripts get `prts.time.tick()` and
  `prts.time.seconds()` (tick × dt), never wall-clock time.

## 2. Entities and components

- **Entity handle**: 32-bit index plus 32-bit generation, shown to Lua as an opaque integer.
- **Built-in components** live in C, in packed arrays per component type:
  `transform` (frame-relative), `frame` (if the entity is a reference frame), `body`
  (velocity, mass), `collider`, `render` (mesh, sprite or voxel), `light`, `audio_source`,
  `door`, `platform` (lifts, decks), `seat`, `trigger`, `script`.
- **Script components**: each entity can carry Lua-owned data in a table that is stored by
  the engine. The data must be **plain** (numbers, strings, booleans, nested tables, handles) so
  it can be saved. Functions and userdata are rejected when saving.
- More component types (weapons, sight, crew, cargo, gathering, production…) arrive with the
  milestones that need them. Each one goes through an OpenSpec change.

## 3. Reference frames

Big moving things with people inside need **nested reference frames**
([concept 04 §4](../concept/04-world-scale-and-theaters.md)).

```
world frame
 └─ behemoth frame           walks, sways and turns with the beast
     ├─ flank lift frame       a rope lift moving up and down the beast's side
     │   └─ supply cart         standing on the lift platform
     │       └─ commander       sitting on the cart
     └─ commander (walking between the huts on its back)
```

- Every entity has a **parent frame** and a **local transform** in single-precision floats,
  relative to that parent. Frames can nest to any depth (practical depth ≤ 6).
- **World positions** use **double precision** (`prts_dvec3`). A 32 km map with a high
  sky 6 km up would show visible float jitter far from the origin, so the world position of each
  frame is kept in doubles.
- **Rendering is camera-relative**: transforms are combined in double precision relative to
  the camera, then turned into float. Precision stays good near the viewer at any distance from
  the origin.
- **Physics runs in the frame of the entity's parent.** The commander walking on a moving deck
  collides with the deck's geometry in deck space, so they do not slip or fall through. The
  frame's own motion is applied when moving to the parent's space.
- **Changing parent**: stepping off a gangplank, onto a lift platform, or across a skyship's
  boarding bridge into a floating citadel changes an entity's parent. Its world transform stays the same and its velocity
  is converted, so there is no pop or jump. Parent changes come from **frame volumes**
  (triggers that say "inside this volume you belong to frame F").
- **Gravity is set per frame**: normal gravity for almost every frame, and none inside
  **weightless volumes** (a floating citadel's aether core). The character controller asks the
  frame for gravity at its position, so other rules can be added without changing it.
- **M1 scope**: the frame tree, camera-relative rendering, parent changes, and the **hoist lift**
  in the keep as the first moving frame. Wagons, siege towers, ships, creatures and citadels come
  in M3–M6.

## 4. Cells and portals (interiors)

- Each interior is split into **cells** (rooms or groups of rooms) joined by **portals**
  (doorways, hatches, windows, open ramps). The outdoor world is one cell as well.
- Cells are **children of their ship's, beast's or building's frame**, so interiors move with their
  hull.
- **Tunnels and deep caverns** use cells and portals too: a tunnel is a chain of cells, and a
  cavern is one or more large cells. Underground theaters are therefore drawn and heard like big
  interiors, while staying part of the one continuous world.
- **Water volumes** (lakes, the sea) are frame volumes with their own rules: buoyancy and
  swimming instead of normal gravity, and short underwater fog for rendering.
- Portals are used for **visibility** (only cells you can see through portals are drawn),
  **audio** (sound passes through open portals and is muffled by closed ones), and later
  **atmosphere** (flooding, fire spread, vacuum) and **crew navigation**.
- A closed door is a portal with `open = false`. Opening it is an animated state change that
  also updates collision.

## 5. Simulation levels of detail

To keep huge, crewed machines affordable (from M3 onward):

| Level | Entities simulated | Example |
|---|---|---|
| **Embodied** | Full entities with transforms, sprites, navigation | Crew of the ship the commander is on |
| **Rostered** | Records only (role, post, state); no transform | Crew of other active units |
| **Aggregated** | Counts and averages per formation | Distant fleet reserves |

Switching levels is **deterministic** and depends only on simulation state (where the commander
is, alert levels, a hysteresis timer), never on rendering. Tests can force any level.

## 6. Determinism

- **Same build, same inputs → same simulation**, bit for bit. This is a **test requirement**:
  replays, scenario tests and future lockstep networking all depend on it.
- How it is kept:
  - Fixed tick; no wall-clock time in simulation code; no reading results of in-flight async
    jobs mid-tick.
  - Seeded **PCG32 RNG streams** per subsystem (`prts.rand.stream("ai")`).
  - Iteration in entity order, never in hash-table pointer order.
  - Lua: the sandbox removes `os.time` and `os.clock`, and `math.random` is replaced by a
    seeded simulation stream. Lua is built with a **fixed string-hash seed** (`luai_makeseed`)
    so that `pairs()` order is the same from run to run within a build. Simulation scripts
    still use `prts.sorted_pairs` wherever order affects results, so that results do not
    depend on the Lua version (see [data-and-scripting.md](data-and-scripting.md)).
  - Floating point: no `-ffast-math`; strict IEEE mode on all compilers.
- **Cross-platform** bit-exact determinism (x86 against ARM against WASM) is **not promised in
  M1**. It will be looked at again if lockstep multiplayer is chosen (open question Q6).
- **State hash**: a 64-bit hash of all simulation state is computed every tick in test builds
  and every N ticks in release builds. Tests compare hash sequences.

## 7. Saves and replays

- **Save** = a versioned binary snapshot: entity store, components, frame tree, RNG states,
  script component tables, plus a list of mounted mods with their versions.
- **Replay** = the starting snapshot plus a stream of **commands per tick** (player inputs that
  affect the simulation, such as movement, use and orders). Look direction is recorded too,
  for playback from the commander's point of view.
- **Replay check**: replaying must give the same state hashes. Automated game tests use this
  heavily ([quality-and-build.md](quality-and-build.md)).

## 8. Networking (future)

Not part of M1–M10 scope (open question Q6). The design keeps both options open:
- **Lockstep** (send commands, the simulation runs on every machine) would need cross-platform
  determinism.
- **Server-authoritative snapshots** (the server simulates and clients interpolate) would cost
  more bandwidth but tolerate floating-point differences.
The command stream used for replays is the same shape that either network model would send.
