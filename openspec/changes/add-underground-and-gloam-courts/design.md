# Design: Hollow Roads (M7)

## Engine: holes and the underground

- **The hole mask.** `prts_heightfield` gains a hole mask, one byte per grid quad.
  - `prts_heightfield_set_hole` marks quads inside an x/z rectangle.
  - `prts_heightfield_hole(hf, x, z)` asks whether a point lies in a hole.
- **Making holes.** `prts.scene.terrain{ holes = { { x0, z0, x1, z1 }, ... } }` marks holes before
  the mesh is built. `prts.scene.hole{ min = { x, z }, max = { x, z } }` marks them later. The
  terrain's index range is kept, so a later hole rewrites its quads as degenerate triangles: the
  index count does not change, and a scene upload follows.
- **Collision.** `clamp_terrain`, `limit_slope` and `prts_cworld_ground` ignore the terrain:
  - where a point lies in a hole;
  - where the box (or probe) lies more than `PRTS_UNDERGROUND` (3 m) under the terrain's surface.

  A fall through solid terrain cannot happen: a falling body moves less than a metre per tick, so
  the clamp catches it in the first tick below the surface.
- **Navigation.** The nav grid's sampling stops at the terrain: a surface more than 3 m under the
  terrain (outside holes) is underground and not ground for units. Over a hole, the ground is
  what lies under it (a mouth's ramp).
- **Queries.** `prts.scene.hole_at(x, z)` answers from Lua.

## Tunnels (`rts/tunnels.lua`)

- **Nodes and galleries.** Nodes are points in the world: mouths, junctions and cavern doors.
  Galleries join two nodes.
  - A gallery is an entity at its midpoint, turned along its direction. It holds a floor (a
    wedge when the ends differ in height), walls and a ceiling of rock, with a glowstone lamp
    every 12 m.
  - Each gallery and cavern is a cell. Galleries are built one after another, so they join; a
    portal is opened at each node.
- **Mouths.** A mouth is a terrain hole with a ramp (a wedge in its own frame) from the surface
  down to the first node, 8 m under it. A gate frame stands at the top.
- **Caverns.** A cavern is a large room of rock boxes with lights and, optionally, a glowstone
  grotto.
- **Routes.**
  - `Tu.route(a, b)` is a breadth-first path over the graph.
  - `Tu.travel(company, a, b)` first sends the members to mouth `a` on the field. As each one
    reaches it, it leaves the agent system: `d.under = { points, i }`.
  - From then on it moves along the route's points at its speed every tick, and it is seen by
    nobody on the surface.
  - At mouth `b` it is put back on the grid. Event `rts_tunnel_out`.
- **Digging.**
  - `Tu.dig(side, from_node, x, z)` opens a dig site. Its length grows each second by
    `DIG_RATE` per digger working there: Gloam delvers 1.0 m/s, other workers 0.2 m/s.
  - When it is done, the gallery and a mouth at the far end are made, with a hole cut there.
  - The Gloam get one stone per metre. While digging goes on, a `rumble` sound is played on the
    surface above it.
- **Collapse.** `Tu.collapse(gallery)`:
  - fills the gallery with rubble boxes;
  - removes it from the graph;
  - hurts every unit travelling in it by half its health;
  - plays the rumble.

  `Tu.open(gallery)` clears it again (Stonemeld).
- **Flooding.** `Tu.flood(gallery)` puts water in a gallery lower than the map's water level.
  - While the commander is inside, they wade (as on the shore).
  - Units travelling through it lose health each second.
  - A map floods a gallery when one of its nodes lies within 30 m of deep water and below the
    water's level.
- **The dark.**
  - While the commander's eye is underground (3 m or more below the terrain), the fog closes in:
    near 3 m, far 40 m, a dark colour.
  - Caverns with glowstone are lit by their lamps.
- **Echo pillars.** `C.in_reach` (the stone's reach): a company travelling underground is in
  reach only within `ECHO_RANGE` (300 m) of one of its side's echo pillars.

## The Gloam Courts (`rts/gloam.lua`)

- **The faction.**
  - Roles as for the other factions: hq, worker, food, foot, cavalry, tower, workshop, aether,
    special, far_sight.
  - The resource drops are a `stone_drop` and a `glowstone_drop`.
  - Stone and glowstone are new side resources; the timber cost of everything is 0.
- **Stone.** Quarries (`E.add_quarry`) are rock outcrops with 3000 stone; delvers gather them like
  gold.
- **Glowstone.** Grottos (`E.add_grotto`) are vents over glowstone caverns. A glowstone works
  built within reach of a grotto makes 1 glowstone every 3 seconds, and aether from the seep.
- **The headquarters.** Delve → Hollow Court → Sunken Throne.
  - On the surface stands a stone gatehouse.
  - Down a stair shaft (a hole, stairs 10 m down) is the hall, with its war room and war table.
    Each stage's hall is bigger.
- **Units** (scaled down as before):
  - delver (worker);
  - basalt guard (8, heavy foot, high armour);
  - deep pikes (8; double damage against mounted units);
  - slingers (8, missile; a struck target is lit, so the side sees it for 30 s);
  - undercutters (4; triple damage to structures);
  - hushfoot (1; seen only within 12 m);
  - echo hounds (5, fast; they see the hushfoot and units travelling in tunnels within their
    sight);
  - carapace riders (4, on beetles, mounted);
  - olm brute (1, big, gate-breaker);
  - moth rider (1, flyer);
  - glimmer-wright (caster, tier 3);
  - mycel keeper (caster, tier 4).
- **The tunnel wyrm** (`props/wyrm.lua`). A mobile item from the wyrm hollow (tier 4): a long
  segmented beast whose harness gondola is a deck plan with a small war table and room for three
  companies.
  - Sent to a point, it dives to 12 m under the ground, burrows there in a straight line at
    5.5 m/s and **bursts up**:
    - a hole is cut at the target;
    - structures within 14 m take heavy damage;
    - its companies are landed on the surface around it.
  - Whoever rides the gondola goes with it. Event `rts_wyrm_burst`.
- **Spells** (`D.SPELLS`, effects in `rts/spells.lua`):
  - **Glowflare** stuns the enemy's units in a radius: they do nothing for 4 s. Durrow and moths
    are immune.
  - **Stonemeld** collapses (or, on a collapsed one, opens) the gallery nearest the target point.
  - **Crystal Sight** shows, for 30 s, every unit within 40 m, including those in tunnels.
  - **Spore Cloud** makes enemies in the area deal half damage and move at half speed for 20 s.
  - **Fungal Mend** heals the side's units in the area each second for 20 s, twice as much
    underground.
  - **Echo Call** shows every enemy within 300 m as an old sighting (rumoured), tunnels
    included.
- **The AI.** The Gloam AI uses the same generic plan, with the Gloam's roles and its train list.

## Four-way matches

- **Sides.** `rts.match.start` takes up to four sides. Unless a side names its `enemy`, every
  other side is its enemy.
- **Helpers.** `S.enemies(side)` lists the living foes; `S.foe(side, x, z)` is the nearest foe's
  headquarters.
- **AI and picture.** The AI's attack and expansion use `S.foe` instead of `side.enemy`. The
  picture already works per side.
- **Losing.** A side without a headquarters has lost: its units stay where they are, and its
  remaining structures stand.
- **Winning.** The match ends when one side, or one alliance, is left.

## Under the Hill (`maps/underhill.lua`)

- **On the surface:**
  - the Crown's camp on Merewatch Hill (a keep, a deep shaft already sunk, a barracks);
  - the Gloam Delve's gatehouse 600 m east, with its Hollow Court underground.
- **Under the ground:**
  - a tunnel road from a sealed gate near the Crown's camp to the Gloam's gatehouse;
  - a deep cavern under the hill with a glowstone grotto and Caldrith's broken chain (huge iron
    links) running down out of the rock.
- **The flow:**
  - at start, the Gloam open the gate;
  - after 3 minutes, a tunnel wyrm bursts up in the Crown's camp, with Gloam guard aboard.
- **The end:** the Crown wins when the Gloam headquarters falls, and loses if its keep falls.

## Hollowfell (`maps/hollowfell.lua`)

- A 2 km square, with a headquarters in each corner: the Crown (the player, south-west), the
  Wildborn (north-west), the Gloam (north-east) and a rival Crown host (south-east, red).
- Quarries, grottos, springs and mines for all.
- A tunnel road runs under the middle, from the Gloam's corner towards the others.
