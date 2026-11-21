# 05 · Units, Structures & Interiors

All sizes are in meters. "Crew" is the number needed for full effectiveness. "Interior" says what
you can walk into. The four playable factions share the classic fantasy RTS roles: worker, foot
soldier, ranged, cavalry, caster, siege, saboteur, flyer, transport ship, warship. Each side fills
those roles in its own way (see [08](08-setting-and-factions.md)), and each has a home ground: the
Crown holds the field, the Wildborn roam the wilds and the sky, the Gloam Courts rule the tunnels
and deep caverns, and the Brine League rules the sea, above and below the waves. Spells are listed
in [14](14-magic-and-spells.md). All values are starting points, set in Lua definitions, and will
be tuned.

| Faction | Units | Building types |
|---|---|---|
| Crown of Esterholt | 22 | 15 |
| Wildborn Clans | 20 | 14 |
| Gloam Courts | 21 | 15 |
| Brine League | 21 | 14 |
| **Total** | **84** | **58** |

Headquarters upgrade chains (for example Holdfast → Keep → High Castle) and tower upgrades count as
one building type. Neutral places are listed in §5.

## 1. The Crown of Esterholt

### Foot, horse and magic (people are sprites)
| Unit | Size | Crew | Role |
|---|---|---|---|
| Serf | 1 person | 1 | Worker: mines gold, fells trees, builds, repairs, fights fires |
| Men-at-arms | Squad of 8 | 8 | Sword and shield; the backbone of the army |
| Pikemen | Squad of 8 | 8 | Stop cavalry, hold a line |
| Crossbowmen | Squad of 8 | 8 | Ranged; fire bolts (upgrade) |
| Sappers | Team of 4 | 4 | Dig under walls, lay powder kegs, clear obstacles |
| Outrider | 1 rider | 1 | Fast scout |
| Knights | Lance of 4 | 4 | Heavy cavalry. Upgrade: **Lantern Knights** with light magic and a healing aura |
| Priest of the Dawn | 1 + acolyte | 2 | Dawn magic: Mend, Blessing, Dawnlight |
| Magister | 1 + apprentice | 2 | Arcane magic: Firebolt, Hush Ward, Bind; far-sight ritual at the Arcanum |

### Engines and wagons
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Supply wagon | 6 | 2 | Driver's bench, covered bed | Arrows, powder, food for an army in the field |
| Command wagon | 8 | 4 | Unfolds into a war tent with a small war table | Mobile command post, stone relay |
| Trebuchet | 15 (arm), 8 wide | 10 | Winch platform, release lever | Breaks walls; assembled where it is used |
| Siege tower | 18 tall, 6 × 6 base | 16 pushers (+30 troops) | Four levels with ladders, drawbridge at the top | Puts troops on enemy walls |
| Dwarven bombard | 5 | 6 | Gun position, powder cart | Cannon against walls and ships |

### Ships
| Unit | Length | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Aether skimmer | 18 | 8 | Deck, pump room | Collects aether from springs in the shallows |
| Cog | 22 | 12 (+60 troops or 2 engines) | Hold, deck, fore and aft castles | Transport |
| Caravel | 25 | 30 | Deck, small hold, master's cabin | Fast warship, scout |
| Fireship | 20 | 6 | Deck packed with pitch | Set alight and steered into the enemy; the crew rows away |
| War galleon | 55 | 250 | Quarterdeck, two gun decks, hold, powder room, chart room, captain's cabin, galley | Heavy warship; naval command post |

### Sky
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Observation balloon | 12 envelope | 2 | Wicker basket | Tethered scout over a winch wagon |
| Dwarven skyship | 45 | 30 (+20) | Gondola deck, bomb bay, engine room, envelope catwalks | Air warship, bomber, transport |
| Sky galleon | 110 | 180 (+80) | Bridge, gun decks, drop bay, crew quarters | Heavy air warship (tier 6); reaches the high sky |

### Buildings
| Building | Footprint | Staff | Interior | Function |
|---|---|---|---|---|
| Holdfast → Keep → High Castle | 40×40 → 80×80 → 140×140; keep tower 25 → 40 m | 30 / 80 / 200 | Great hall, **war room**, treasury, chapel, kitchens, cellars | Headquarters; gold and timber are delivered here; succession anchor |
| Farmstead | 30 × 30 | 6 | Farmhouse, barn | Food (feeds 8 folk) |
| Barracks | 40 × 30 | 20 | Bunk hall, armory, yard | Foot soldiers |
| Sawmill | 40 × 25 | 12 | Saw hall with a water wheel | Timber delivery point, timber upgrades |
| Smithy | 20 × 15 | 6 | Forge, anvils | Weapon and armor upgrades |
| Stables | 50 × 30 | 12 | Stalls, tack room | Cavalry |
| Arcanum | 15 × 15, 35 m tower | 8 | Library, scrying pool, observatory | Magisters, far-sight |
| Chapel of the Dawn | 25 × 15 | 6 | Nave, bell tower | Priests, Lantern Knight upgrade |
| Dwarven Forgeworks | 60 × 40 | 30 | Casting floor, powder store | Bombards, siege engines, skyship parts |
| Harbor | 120 × 60 + docks | 60 | Slipway, rope walk, warehouses | Ships |
| Skyship mast | 60 m tower | 15 | Mooring platform, lift cage | Builds and docks skyships |
| Watchtower → Arrow tower / Bombard tower | 8 × 8, 18 m | 2–6 | Ladder, platforms | Sight and defence |
| Walls and gatehouse | Stone, 8 m high | — | Wall walk, gate winch room | Defence |
| Beacon spire | 6 × 6, 30 m | 2 | Relay chamber at the top | Sending-stone relay |
| Aether well | 20 × 20 | 10 | Pump house | Aether on land |

## 2. The Wildborn Clans

### Foot, beasts and magic (sprites)
| Unit | Size | Crew | Role |
|---|---|---|---|
| Kobb | 1 small person (1.1 m) | 1 | Worker: mines, fells trees, builds, repairs |
| Ghorr brutes | Squad of 6 | 6 | Heavy melee infantry |
| Spearhurlers | Squad of 6 | 6 | Ranged; thrown javelins |
| Kobb firebrands | Team of 4 | 4 | Set buildings and ships on fire, cut ropes and lifts |
| Wolf runner | 1 rider | 1 | Fast scout |
| Boar riders | 4 riders | 4 | Heavy cavalry on great boars |
| Shaman | 1 | 1 | Spirit magic: Spirit Boar, War Chant, Stillsong |
| Bone-seer | 1 + 2 bearers | 3 | Bone magic: Wither, Spirit Crows, Grave Fog; far-sight ritual at the Spirit lodge |
| Ridge giant | 6 m tall | 1 + 2 Kobb handlers | Siege: throws boulders, breaks gates |

### Engines and beasts
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Pack beasts | 4 | 2 | — | Field supplies |
| Battering ram | 12 (covered shed) | 12 | Under the roof, by the ram | Breaks gates |
| **Behemoth** | 50 long, 22 tall | 140 | A war camp of huts, ramps and a howdah war room on the beast's back | Moving War Lodge; command post; succession anchor (Wildborn signature) |

### Water
| Unit | Length | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Aether raft | 15 | 8 | Deck, collecting nets | Collects aether from springs in the shallows |
| Longship | 28 | 60 (+20) | Open deck, small hold | Raider and transport |
| Ram galley | 35 | 80 | Rowing deck, ram room | Rams and sinks ships |
| Serpent rider | 30 (serpent) | 1 | — | Attacks unseen from below |
| Bonehulk | 80 | 300 | Decks built inside the ribcage of an ancient leviathan, a war-drum hall, holds | Heavy warship; naval command post |

### Sky
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Wyvern rider | 14 wingspan | 1 | — | Fast flyer, hunts balloons and riders |
| Roc | 32 wingspan | 2 (+12) | Woven basket slung under the bird | Transport |
| Elder drake | 40 wingspan | 1 drake + 6 in the war howdah | Howdah on the drake's back | Breathes fire (tier 6); reaches the high sky |

### Buildings
| Building | Footprint | Staff | Interior | Function |
|---|---|---|---|---|
| Longhouse → War Lodge → Antler Throne | 30×50 → 60×80 → 100×120 | 30 / 80 / 200 | Feast hall, **war room** (a war table of bone and hide), treasure pit, fire pits | Headquarters; delivery point; succession anchor |
| Herd pens | 40 × 40 | 6 | Herder's hut, pens | Food (Wildborn herd, they don't farm) |
| War pit | 40 × 40 | 20 | Fighting pit, sleeping dens | Foot warriors |
| Timber camp | 30 × 20 | 10 | Log yard, cutting shed | Timber delivery point |
| Bone forge | 25 × 20 | 8 | Forge pit, bone racks | Upgrades, battering rams |
| Beast den | 50 × 40 | 12 | Kennels, sties | Wolves and boars |
| Spirit lodge | 20 × 20 | 6 | Smoke hall, seeing pool | Shamans, bone-seers, far-sight |
| Giant's cairn | 40 × 40 | 4 | Stone circle, the giants' cave | Ridge giants |
| Raft-yard | 100 × 50 + beach | 50 | Slips, drying racks | Ships |
| Roost cliffs | 40 × 40 + rock spire | 15 | Cave nests, climbing ropes | Wyverns, rocs; elder drakes at tier 6 |
| Totem watch | 6 × 6, 14 m | 2 | Climbing pegs, platform | Sight and defence |
| Palisade and gate | Timber, 5 m | — | Fighting platform | Defence (it burns) |
| Drum tower | 8 × 8, 20 m | 3 | Drum platform | Sending-stone relay |
| Aether shrine | 20 × 20 | 10 | Shrine cave | Aether on land |

## 3. The Gloam Courts

### Foot, beasts and magic (sprites)
| Unit | Size | Crew | Role |
|---|---|---|---|
| Delver | 1 person (1.4 m) | 1 | Worker: mines gold, cuts stone and glowstone, digs tunnels, builds |
| Basalt guard | Squad of 8 | 8 | Heavy infantry with tower shields; holds tunnels and gates |
| Deep pikes | Squad of 8 | 8 | Crystal-tipped pikes; stop cavalry and beasts |
| Gloam slingers | Squad of 8 | 8 | Ranged; glowstone shot that keeps glowing on what it hits, so targets stay visible in the dark |
| Undercutters | Team of 4 | 4 | Dig under buildings and walls and collapse them from below |
| Hushfoot | 1 | 1 | Stealthy scout; sees in the dark and is hard to spot |
| Echo hounds | Pack of 4 + handler | 5 | Fast, blind cave hounds that hunt by sound; track hidden units |
| Carapace riders | 4 riders | 4 | Heavy cavalry on giant cave beetles |
| Glimmer-wright | 1 + apprentice | 2 | Crystal magic: Glowflare, Stonemeld, Crystal Sight; deep-sight ritual at the Crystal sanctum |
| Mycel keeper | 1 | 1 | Fungus magic: Spore Cloud, Fungal Mend, Echo Call |
| Olm brute | 8 m long | 1 + 2 Durrow handlers | A giant blind cave salamander; breaks gates, swims in underground lakes |

### Engines and beasts
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Pack beetle | 5 | 2 | Saddle frame | Field supplies |
| Spore mortar | 4 | 6 | Loading platform | Lobs spore shells that sicken defenders |
| Lumen ballista | 6 | 4 | Firing deck | Glowing bolts; targets it hits stay lit for every Gloam unit to see |
| Glow-drill | 14 | 8 | Driver's cab, crystal drill chamber | Bores tunnels fast; breaks into buildings from below |
| **Tunnel wyrm** | 70 long, 4 thick | 3 handlers (+30 troops) | Harness gondola with a small war table | Burrows under the map with troops aboard and bursts up under buildings (Gloam signature) |

### Water (rivers and underground lakes)
| Unit | Length | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Lake barge | 20 | 8 (+40 troops or 1 engine) | Deck, hold | Transport |
| Ram-skiff | 12 | 10 | Open deck, ram | Fast attacker |
| Shell galley | 36 | 90 | Rowing deck, armored upper deck under beetle-shell plates, small chart room | Armored warship |

### Sky
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Moth rider | 10 wingspan | 1 | — | Night scout and raider; weaker in bright sunlight |
| Lantern moth | 24 wingspan | 2 (+8) | Wicker howdah with a glowstone lamp | Night transport; drops flares or spore bombs |

### Buildings
| Building | Footprint | Staff | Interior | Function |
|---|---|---|---|---|
| Delve → Hollow Court → Sunken Throne | Surface gatehouse 30 × 30; halls 60×60 → 90×90 → 120×120, 40–120 m below ground | 30 / 80 / 200 | Gate, stair shafts, carved halls, **war room** with a stone war table, treasury vault | Headquarters; delivery point; succession anchor. Reached from the surface only through its gate |
| Mycel gardens | 40 × 40 cavern | 6 | Fungus beds, drying racks | Food (feeds 8 folk) |
| Guard hall | 40 × 30 | 20 | Barracks hall, training gallery | Foot soldiers |
| Stonecutters' hall | 40 × 25 | 12 | Cutting floor, stone sledges | Stone delivery point, tunnel upgrades |
| Glowstone works | 25 × 20 | 8 | Crystal kilns, cutting benches | Glowstone delivery point; collects aether from seeps |
| Beetle pens | 50 × 30 | 12 | Pens, harness room, hound kennels | Carapace riders, pack beetles, echo hounds |
| Crystal sanctum | 20 × 20 | 6 | Crystal hall, deep-sight pool | Glimmer-wrights; deep-sight ritual |
| Spore grotto | 20 × 20 | 6 | Damp grotto, spore vats | Mycel keepers |
| Engine vault | 50 × 40 | 25 | Assembly vault, drill bay | Spore mortars, lumen ballistae, glow-drills |
| Wyrm hollow | 60 × 60 | 15 | Wyrm pit, handlers' galleries, olm pool | Tunnel wyrms, olm brutes |
| Lake dock | 60 × 30 + jetty | 30 | Slipway, rope store | Barges, skiffs, shell galleys |
| Moth spire | 12 × 12, 30 m hollow spire | 10 | Spiral roosts inside the spire | Moth riders, lantern moths |
| Tunnel gate | 10 × 10 | 4 | Gate chamber, rockfall trap | Fortified tunnel mouth; can be sealed or collapsed |
| Glow tower → Lumen tower | 6 × 6, 16 m | 2–4 | Stair, lamp chamber | Sight and defence; the Lumen tower's beam blinds attackers |
| Echo pillar | 4 × 4, 12 m | 1 | Resonance chamber | Sending-stone relay that also works underground |

## 4. The Brine League

### Foot, beasts and magic (sprites)
| Unit | Size | Crew | Role |
|---|---|---|---|
| Dockhand | 1 person | 1 | Worker on land: mines gold, fells trees, builds, repairs ships |
| Pearl diver | 1 Hesk | 1 | Worker under water: gathers pearls and aether, salvages wrecks |
| Marines | Squad of 8 | 8 | Cutlass and buckler; boarding and landings |
| Harpooneers | Squad of 6 | 6 | Ranged; harpoons that pin beasts, flyers and boats |
| Reef guard | Squad of 6 Hesk | 6 | Amphibious melee; fights in and out of the water |
| Hull-borers | Team of 4 Hesk | 4 | Swim under ships and harbors, bore holes, cut chains and anchor lines |
| Crab riders | 4 riders | 4 | Heavy amphibious cavalry on giant reef crabs |
| Weatherwright | 1 + apprentice | 2 | Storm magic: Squall, Lightning Lash, Fog Bank; storm-eye ritual at the Storm tower |
| Tidecantor | 1 Hesk | 1 | Tide magic: Tide Surge, Brine Mend, Undertow |

### Engines
| Unit | Size | Crew | Interior | Role |
|---|---|---|---|---|
| Harpoon ballista | 6 | 4 | Firing deck, winch | Anti-beast and anti-ship engine, on land or on deck |

### Ships
| Unit | Length | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Pearl lighter | 16 | 8 | Deck, diving platform | Collects pearls and aether from springs |
| Sloop | 20 | 25 | Deck, small cabin | Fast warship, scout |
| Cargo hulk | 35 | 20 (+80 troops or 3 engines) | Holds, loading ramp | Transport |
| Brigantine | 32 | 60 | Ballista deck, hold, master's cabin | Warship; boarding |
| Bomb ketch | 26 | 30 | Mortar well, magazine | Lobs sea-fire pots at coasts and harbors |
| War carrack | 65 | 400 | Three decks, ballista batteries, chart room, holds | Heavy warship; naval command post |
| **Tidehold** | 150 × 120 raft-city | 600 | Docks, market, workshops, council deck with a great war table | Moving naval headquarters; succession anchor (League signature) |

### Under water
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Manta riders | 3 riders on 8 m rays | 3 | — | Fast underwater cavalry |
| Diving bell | 6 tall | 2 (+12) | Air-filled bell with a small shell-and-sand table | Carries troops under water; underwater command post |

### Sky
| Unit | Size | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Albatross riders | 12 wingspan | 1 | — | Scout and fighter over the sea |
| Wind-barque | 60 | 70 (+30) | Deck, rigging, cabins | Sailing airship that rides the storm layer's winds |

### Buildings
| Building | Footprint | Staff | Interior | Function |
|---|---|---|---|---|
| Wharf hall → Guildhall → Admiralty | 40×40 → 70×70 → 120×100, always on a shore | 30 / 80 / 200 | Counting hall, **war room** with a war table of inlaid shell, treasury, private dock | Headquarters; delivery point; succession anchor |
| Fishery | 40 × 30 + weirs | 8 | Smokehouse, net loft | Food (feeds 8 folk) |
| Muster house | 40 × 30 | 20 | Bunk hall, drill yard | Marines, harpooneers |
| Timber yard | 50 × 30 | 12 | Ropewalk, saw pit | Timber delivery point, ship upgrades |
| Pearl house | 25 × 20 | 8 | Sorting hall, aether vats | Pearl and aether delivery point |
| Shipwright yard | 100 × 50 + slip | 50 | Slipways, sail loft | Lighters, sloops, hulks, brigantines, bomb ketches, diving bells |
| Great dock | 160 × 60 | 120 | Dry dock, cranes | War carracks, wind-barques, the Tidehold |
| Reef pens | 50 × 40, half under water | 10 | Tide pools, crab pens | Crab riders, manta riders, reef guard |
| Storm tower | 12 × 12, 30 m | 6 | Weather loft, storm-eye pool | Weatherwrights; storm-eye ritual |
| Tide temple | 30 × 20, half under water | 6 | Flooded nave | Tidecantors, pearl divers, hull-borers |
| Harpoon battery | 12 × 12 | 6 | Ballista platform, winches | Shore defence |
| Sea wall and chain boom | Stone wall + 200 m chain | — | Wall walk, chain winch house | Harbor defence; the chain closes the harbor mouth |
| Signal mast | 6 × 6, 25 m | 2 | Lamp top, relay chamber | Sending-stone relay; long-range signal lanterns |
| Albatross aerie | 30 × 30 on a cliff | 10 | Cliff nests, perches | Albatross riders |

## 5. Neutral places

| Place | Size | Interior | Notes |
|---|---|---|---|
| **Gold mine** | Headframe plus a 60 m shaft and galleries | Lift cage, galleries, ore carts, a foreman's office | Claimed by building a mine house over it. Runs out |
| Villages | 5–20 buildings | Houses, inn, mill | Provide folk (see [06](06-economy-and-logistics.md)) |
| Ruins of the Imperium | Various | Halls, vaults | Aether, old secrets, Custodian guardians (later) |
| **Floating citadel** | 800 across, 300 tall | Throne hall with a scrying floor, barracks, docks, drop-gates, the weightless **aether core** | Garrison of up to 2,000. Drifts high above the map. See [04 §3](04-world-scale-and-theaters.md) |
| Market town | 30–60 buildings inside walls | Market square, guild halls, inn, council hall | Trade resources at changing prices. The holder gets folk and the traders' rumor network (a command post, see [02](02-the-commander.md)) |
| Lighthouse | 12 m base, 45 m tower | Keeper's house, stair, lamp room | Long sight over the sea at night; signal relay; a command post when held |
| Sunken wreck | An Imperium sky-barge, 120 m long, in 20–40 m of water | Flooded holds, trapped air pockets | Aether and relics; reached by divers and diving bells |
| Glowstone grotto | Cavern 100–300 m across | Crystal galleries, an underground lake | Rich glowstone; the most fought-over place in the deep caverns |

## 6. Interiors

### Interior classes
| Class | Examples | How it is made |
|---|---|---|
| Small | Wagon bench, roc basket, balloon basket, ram shed | Hand-built low-poly model with seat and post markers |
| Compartment | Siege tower, command wagon tent, drake howdah | Hand-built model with posts |
| Multi-room | Barracks, sawmill, caravel, skyship | Hand-built model or a deck plan |
| Great | Keeps and castles, the Sunken Throne, galleons, bonehulks, war carracks, the Tidehold, behemoths, mines, floating citadels | **Deck plans** plus **hero rooms** |
| Underground | Tunnels and deep caverns | Tunnel kit and cavern pieces (later: voxel chunks that can be dug at run time) |

### Deck plans
Nobody can hand-build every room of a High Castle or every cabin of a 300-crew bonehulk. Large
interiors are described as **deck plans**: grid-based layouts (2 m cells, one plan per floor or
deck). Each character in the plan maps to a piece of a retro **modular kit** (walls, doors,
hatches, stairs, ladders, beds, barrels, cannons, tables). The engine turns a deck plan into
geometry, collision, navigation for the crew and room data. **Hero rooms** (war rooms, throne
halls, the aether core, a galleon's gun deck) are hand-built and placed into the plan. Deck plans
are text files, so they are easy to mod (see [11](11-modding.md)).

### Why interiors matter (Pillar P4)
- **Command posts** are interiors: war rooms, chart rooms, howdahs, the throne hall.
- **Rooms are the damage model.** Each ship, engine and building has **systems** (sails and
  oars, guns, the powder room, the stone relay, command, the lift, the aether core), and each
  system lives in rooms and posts. Flood the powder room and the guns go quiet.
- **Work** happens in places you can see: the mine's galleries, the sawmill's saw hall, the
  forge's casting floor, the harbor's slipway.
- **Crew posts** decide what a unit can do. An unmanned cannon does not fire, and a behemoth
  without its drivers will not walk.
- **Storming** (later milestone): troops climb in through a breach or over a siege tower's
  drawbridge and take a castle or ship room by room until the war room falls.

## 7. Crew

### Personnel records
Every crew member is a **roster record**: name, people (human, dwarf, Ghorr, Kobb, Durrow, Hesk), role, rank,
skill (0–100), experience, morale and fatigue. People come from the folk your farms and herds
can feed (see [06](06-economy-and-logistics.md)). Survivors keep their experience.

### Roles
Captain, sergeant, soldier, archer, rider, gunner, sailor, bosun, carpenter, sailmaker, beast
handler, driver (behemoth), wyrm handler, diver, skyship engineer, mage, healer, cook, miner,
sawyer, stonecutter, smith…
Roles decide which posts a crew member can take and how well they do it.

### Crew levels of detail
Thousands of people cannot all be simulated in full at once, so crew use three levels of detail:

| Level | When | Simulated as |
|---|---|---|
| **Embodied** | The commander is inside or close to the unit | Visible sprites that walk the deck plan, man their posts, run to the guns and fight fires |
| **Rostered** | The unit is in normal play elsewhere | Records assigned to posts and rooms, with no positions |
| **Aggregated** | Distant or idle companies | Numbers per role, plus average skill and morale |

Switching levels is deterministic. When you board a galleon, the crew appear at the posts the
roster says they are at, depending on the watch and the alarm state. During a battle you will
always find the master gunner on the gun deck.

### Damage control loop (ships, castles, behemoths, citadels)
Hit → room event (fire, flooding, collapse, smoke) → the bell or drums sound → firefighting and
repair crews move through the deck plan → repairs take time and skill → systems come back. If the
commander is present, this is faster (presence), and you can **choose which rooms come first**
from the quarterdeck or war room.

## 8. Production is physical

- **Serfs, Kobb, delvers and dockhands carry** gold in sacks from the mine, logs from the forest
  and cut stone from the tunnels, and pearl divers swim up with nets of pearls. You can see them
  all walk (or swim) to the delivery point. Ambushing the gatherers hurts.
- Soldiers drill in the barracks yard and march out through the gate. Knights lead their
  horses out of the stables.
- Ships are built on the harbor slipway and launched. Skyships are rigged at the mast. Engines
  are wheeled out of the Forgeworks.
- Every unit needs **folk** to crew it before it can leave the building.
- You can walk onto a half-built ship on the slipway, or into a keep that is still in
  scaffolding, and watch the work.
