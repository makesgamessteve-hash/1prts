# 06 · Economy & Logistics

The economy is classic fantasy RTS (workers gather gold and timber, food limits your army) and
it is physical. Every sack and every log is carried by someone you can see, along a path that
can be raided.

## 1. Resources

| Resource | Source | Carried by | Delivered to | Used for |
|---|---|---|---|---|
| **Gold** | Gold mines (they run out) | Serfs / Kobb, in sacks | Holdfast/Keep/Castle, Longhouse/Lodge/Throne | Almost everything |
| **Timber** | Trees (forests shrink as they are cut) | Serfs / Kobb, as logs | Headquarters, Sawmill, Timber camp | Buildings, ships, engines, arrows |
| **Aether** | Aether wells and shrines on land; springs in the shallows (skimmers and rafts) | Wagons, skimmers, rafts | Headquarters, Arcanum, Spirit lodge | Magic, Lantern Knights, skyships, drakes, waking citadels |
| **Food** | Farmsteads (Crown), herd pens (Wildborn), captured villages | — (capacity, not stored) | — | Sets how many **folk** you can support |
| **Folk** | Villages and housing, within the limit food sets | On foot | Barracks and other trainers | Crewing every unit, building and ship |

There is no stone or iron. Like the genre it comes from, the economy stays readable with two
main resources plus one rare one.

## 2. Folk and food: the army's limit

- **Food** works as a cap. Each farmstead or herd pen feeds a set number of folk. Lose a farm
  and you can't train more units until you rebuild it. Units you already have still fight.
- **Folk** are real people. They join from villages you control (a slow, steady trickle) and
  from your farmsteads and herds. Training turns them into soldiers, sailors, mages and riders.
- Every unit needs a full crew before it is finished: 1 for a serf, 10 for a trebuchet, 250 for
  a war galleon, 140 for a behemoth. Losing a galleon costs folk as well as gold and timber.
- Survivors gain experience. Rescuing a downed wyvern rider or carrying the wounded back to the
  infirmary is worthwhile.

## 3. Gathering and delivery

- **Gold**: workers walk into the mine (you can follow them down), come back up with a sack and
  carry it to the nearest delivery point. Mines slowly run out, so you have to expand.
- **Timber**: workers fell trees one by one. Forests shrink over a match, which opens new paths
  and new lines of sight. A sawmill or timber camp near the forest shortens the walk.
- **Aether**: wells and shrines are slow but steady. Springs in the shallows are rich but
  exposed, and skimmers and rafts must be protected by ships.
- **Delivery points** decide your gathering speed. Building them close to the resource is the
  core economic skill, as in classic RTS.
- **Field supplies** (optional match rule): armies far from a delivery point use up arrows,
  powder and food. Supply wagons and pack beasts carry more. A company without supplies loses
  morale and effectiveness.

## 4. Construction

1. **Choose** a building on the field map or at the war table.
2. **Place** it with the marking staff's outline at a site you can see, or at the war table on
   land that has been scouted. The outline checks slope, space and where it is allowed (a harbor
   needs a shore, a mine house needs a gold mine).
3. **Workers** are sent and bring the gold and timber.
4. **Building stages**: staked out → foundations → frame and scaffolding → walls and roof →
   finished. You can walk in from the frame stage onward.
5. **Presence** speeds up building by 25% (see [02](02-the-commander.md)).

Wooden buildings are cheap and burn. Stone buildings (keeps, castle walls, towers) cost more
gold and take longer, and they do not burn.

## 5. Tiers and upgrades

Progress comes from **upgrading your headquarters** and building the right workshops, the way
classic fantasy RTS does it:

| Tier | Crown | Wildborn | Opens |
|---|---|---|---|
| T1 | Holdfast | Longhouse | Workers, basic foot, ranged, scouts, farms/herds, barracks/war pit, sawmill/timber camp |
| T2 | Keep | War Lodge | Cavalry, siege, ships, towers, the first magic, smithy/bone forge upgrades |
| T3 | High Castle | Antler Throne | Advanced magic, skyships/sky galleons or rocs/elder drakes, waking a floating citadel |

**Upgrades** are bought at workshops: better blades and armor (smithy / bone forge), fire bolts,
longer-range sending stones (Arcanum / Spirit lodge), faster serfs, stronger walls. All of this
is defined in Lua, so mods can build any tech tree they want.

## 6. Economic pacing (1v1, M map, reference values)

| Time | Typical state |
|---|---|
| 0–5 min | Headquarters, first mine and sawmill, two farms, scouts |
| 5–15 min | Barracks, more workers, first company of soldiers, a forward watchtower |
| 15–30 min | Tier 2, cavalry, first siege engine, harbor, beacon spires or drum towers |
| 30–50 min | Fleets and first flyers; second mine; raids on enemy gatherers |
| 50+ min | Tier 3, skyships or drakes, the race to wake a floating citadel, deciding assaults |
