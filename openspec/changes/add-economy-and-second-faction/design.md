# Design: the full economy and the Wildborn

## Factions (`rts/defs.lua`, `rts/wildborn.lua`)

`D.FACTIONS[f]` names the definitions that fill each role:
- `hq` (a list by tier);
- `worker`, `food`, `foot`, `cavalry`, `tower`, `timber_drop`, `workshop`, `aether`;
- `special` (powder or bone);
- `ai`: the build order and the train lists.

Each side has `faction`. `S.hq(side)` is the side's best finished headquarters (highest tier,
then the oldest). A mobile headquarters (the behemoth) counts, ranked as T2.

## Resources

- `S.RESOURCES = { "gold", "timber", "aether", "powder", "bone" }`. Costs may name any of them;
  `S.can_pay` and `S.pay` check them all.
- **Aether:** a spring is a resource node (`kind = "spring"`). Wells and shrines must be placed
  within 6 m of a free spring; each finished one adds 1 aether every 2 s.
- **Powder:** a finished Forgeworks turns 10 gold + 10 timber into 5 powder every 10 s while the
  side's stock allows and powder is under 400.
- **Bone:**
  - a finished herd pen adds 1 bone every 5 s;
  - a unit that dies leaves **remains** (a node with bone 4 per folk, at most 20), which Kobb
    gather like gold and deliver to a headquarters;
  - remains vanish after 3 minutes.

## Tiers and mandates (`rts/tiers.lua`)

- `side.tier` is the highest of the HQ's tier and the mandates bought.
- A definition's `tier` (default 1) must be reached to train or place it.
- Upgrading the HQ is an order at the HQ (`T.upgrade_hq(e)`): it pays the next stage's cost and
  takes its build time. The entity is then rebuilt as the next stage, keeping health in
  proportion.
- Mandates (`D.MANDATES`) have `tier`, `cost` and `needs` (definition ids that must stand
  finished, or tiers); they are bought at a T3 HQ.

## Upgrades (`rts/tiers.lua`)

`D.UPGRADES = { { id, name, at, tier, cost, time, units = { ... }, damage = n, ignite = bool } }`.
- Researched in the workshop's queue like training.
- `side.upgrades[id] = true` when done.
- Combat asks `T.damage_bonus(side, unit_def)` and `T.ignites(side, unit_def)`.

## Construction stages (`rts/structures.lua`)

- `stage = 1 + floor(progress / build * 4)` while building:
  1. staked;
  2. foundations;
  3. frame;
  4. walls.
- The site's look is a child entity rebuilt when the stage changes. Nothing on a site is solid,
  so the commander can walk in from the start; stage 3 onward is walkable.
- Presence: a builder within 50 m of its side's commander works 25% faster.

## Fire (`rts/fire.lua`)

- Fire only affects structures with `material = "timber"` (the default for timber defs).
- `F.ignite(e, n)` raises `fire` by n, up to 10. A child entity holds flame boxes, one per 2
  intensity.
- Twice a second:
  - **Damage:** a burning structure loses `4 × fire` hp.
  - **Spread:** at fire ≥ 3, it ignites timber structures whose footprints are within 8 m (+1).
  - **Burning up:** with no one fighting it, fire grows by 1 every 4 s up to 10.
  - **Firefighting:** each worker on the "fire" task within reach lowers it by 0.5 per second
    (×1.2 with presence).
- Each burning structure draws up to 3 of its side's nearest idle or gathering workers within
  80 m onto the fire task. At 0 the fire is out and the flames go.
- Sources of fire:
  - Kobb firebrands' strikes on structures (+2 each);
  - crossbowmen with **fire bolts** (+1 per hit on a structure).

## Presence (`rts/presence.lua`)

`Pr.near(side, x, z)` is true if that side is led by the commander and they stand within 50 m.
- Companies with presence break at a third of their strength (not half), and strike for +15%.
- Builders work 25% faster, firefighters 20% faster.

## Succession (`rts/succession.lua`)

- The commander has `hp` (100) in the command state. Enemy units within 20 m may take the
  commander as a target (preferred over units when within reach). Each strike lowers `hp` by the
  weapon's damage.
- The commander gets back 2 hp/s after 10 s without being hit.
- At 0 hp:
  - the commander falls and the screen fades: *"Your banner passes to your second."*;
  - movement is frozen for `delay` (default 30 s);
  - then the successor wakes at the war table of the best anchor: an HQ of stage 2 or more,
    or a behemoth;
  - `successors` (default 3) goes down by one;
  - the stones are silent for `handover` (60 s): `C.send` answers *"the stones are silent"*.
- With no successor or no anchor, the side has lost.

## The Wildborn (`rts/wildborn.lua`)

Units:

| Unit | Squad | Notes |
|---|---|---|
| Kobb | 1 | Worker, gathers bone too |
| Ghorr brutes | 6 | Melee |
| Spearhurlers | 6 | Range 22 m |
| Kobb firebrands | 4 | Ignite structures; T2 |
| Wolf runner | 1 | Scout |
| Boar riders | 4 | Charge; T2 |
| Ridge giant | 1 | Triple damage to structures; T3 |
| Behemoth | 1 | T3 here (the concept's T5 waits for the mandates' content) |

Buildings, mostly timber:
- longhouse → war lodge → antler throne (the headquarters stages);
- herd pen (food 10, bone);
- war pit, beast den, timber camp, bone forge;
- totem watch (sight, a hub, javelins);
- giant's cairn (stone);
- aether shrine.

## The behemoth (`props/behemoth.lua`)

- A structure definition with `mobile = true` and `hq = true`. Its entity carries the beast:
  - body, legs and head as boxes, 30 m long and 13 m to its back;
  - a back deck with two huts and a howdah war room with the war table (for a Wildborn
    commander);
  - a ramp of steps up its left flank.
- It does not block the nav grid. Its footprint follows it, for deliveries, targeting and reach.
- `Be.order(e, x, z)` walks it there at 1.2 m/s, turning at 0.15 rad/s. Riders and the
  commander on it are carried.

## The AI (`rts/ai.lua`)

- Generic over factions through `D.FACTIONS[f].ai`:
  - `build`: an ordered plan of `{ role or id, count, when }`;
  - `train`: lists by tier;
  - `upgrades`;
  - `hq_up`: the gold at which it upgrades.
- Each think it takes the first plan entry that is short and affordable.
- Aether: it places its aether site at the nearest free spring.
- Expansion: when the gold left in mines within 150 m of its HQ falls under 25% of what they
  started with, it builds a stage-1 HQ beside the richest mine that no side is using.

## Thornmere (`maps/thornmere.lua`)

- 1.6 km × 1 km:
  - Crown holdfast at x −600, Wildborn longhouse at x +600;
  - three mines and two aether springs near each base;
  - three mines and one spring in the middle;
  - forests along both edges and a stream.
- Player Crown, AI Wildborn by default. `M.SIDES` can swap them, and scenarios do.
