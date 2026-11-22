# Design: Deep Water (M8)

## The fog arbiter (`lib/fog.lua`)

The storm (M6), the tunnels (M7) and now the water each change the fog. `Fog.base(near, far,
color)` is the map's own fog. `Fog.set(layer, near, far, color)` and `Fog.clear(layer)` add or
remove a layer. The layer with the highest priority applies (underground > underwater > storm >
base), and nothing is sent to the renderer unless the result changes. A fog-only env call is
cheap (M6).

## Swimming, diving and breath (`rts/water.lua`)

- **Swimming.** Where the water is deeper than 1.5 m and the commander's feet are under the
  surface, the commander swims. The script holds them in the water each tick, as in the aether
  core:
  - jump rises (at most to 1.1 m under the surface: treading water);
  - crouch dives at 1.2 m/s;
  - walking glides at 1.6 m/s where the view points;
  - the seabed (terrain) stops them.
- **Breath.** `BREATH` is 30 s, or 120 s with a breath charm: the commander's side is the League,
  or has the `brine.breath_charm` upgrade.
  - While the eye is under the surface and not in an air pocket, breath runs down.
  - At 0 the commander loses 10 health a second (`rts/succession.lua`'s hurt).
  - It refills at 10/s at the surface or in air.
  - The HUD shows a breath bar while it is not full: a minimal, non-diegetic exception, like the
    subtitles.
- **Air pockets.** `Wa.air(entity, min, max)` registers a box in an entity's frame: the bell's
  inside, or a wreck's air-filled hold. Inside one there is no swimming and no loss of breath.
- **Underwater fog.** While the eye is under the surface (and not in air) the fog layer
  `underwater` is near 1, far 30, blue-green. Event `underwater`.

## Reefs, pearl beds and wrecks (`rts/economy.lua`, `props/wreck.lua`)

- **Pearl beds.** `E.add_pearl_bed(x, z)` places coral on the seabed and gives it a work spot on
  the nearest shore where the water is 0 deep and the grid is walkable. Pearl divers gather
  `pearls` there like gold. Pearl houses and League headquarters take them.
- **The wreck.** `Wreck.spawn{ name, x, z, yaw }`: a broken sky-barge on the seabed, whose
  after-hold is an air pocket. Its relic chest is usable once: it gives the diver's side 150
  aether. Event `wreck_relic`.

## The diving bell (`props/bell.lua`)

- **Making it.** `Bell.make{ name, x, z, yaw }` stands a crane on the shore, or on a jetty, with
  the bell hanging from it.
- **Inside.** The bell is a 3 × 3 m open-bottomed room with a lever and a small shell table (a war
  table, `Tools.furnish(bell, base)`). The commander steps in through the open side.
- **The lever.** It lowers the bell to the seabed, or raises it again, at 1.2 m/s. Its inside is
  an air pocket.

## The Brine League (`rts/brine.lua`)

- **The faction.**
  - Roles as for the others; `gathers = { "gold", "timber" }`, plus pearl divers for pearls.
  - The headquarters stands within 30 m of water deeper than 2 m (`shore = true`).
- **Units:**
  - dockhand (worker);
  - pearl diver (worker; gathers `pearls`; `diver`);
  - marines (8);
  - harpooneers (6, missile; a struck target is pinned at half speed for 5 s);
  - reef guard (6, armoured melee);
  - hull-borers (4; they strike ships within 8 m of the shore, ×4 against hulls);
  - crab riders (4, mounted);
  - albatross rider (flyer);
  - Weatherwright (caster, tier 3);
  - Tidecantor (caster, tier 4).
- **Ships** (`D.SHIPS`): pearl lighter (cargo 1), sloop (fast, light guns), cargo hulk (cargo 6),
  brigantine (guns), bomb ketch (guns ×3 against structures), war carrack (heavy guns, a chart
  room command post). All are built at the shipwright yard, the carrack at the great dock.
- **The Tidehold** (`props/tidehold.lua`). A mobile headquarters (`mobile = true, sails = true`)
  built at the great dock, or placed by a map:
  - a 40 × 30 m raft of decks with huts, a market and a council deck with a great war table;
  - it sails at 1.0 m/s on water (`Be.move_all` keeps mobile structures moving; sailing ones
    float at the water's level);
  - broadsides hit it like any building.
- **The wind-barque.** An airship kind (`props/skyship.lua`) built at the great dock. Inside the
  storm band it flies 2.5 times faster.
- **Spells:**
  - **Squall:** a lane of wind puts out fires, knocks enemy flyers in it down to the ground
    (damage), and pushes enemy ships along the lane.
  - **Lightning Lash:** damages a target and jumps to the two nearest enemies within 15 m; 1.5×
    on water or in the storm band.
  - **Fog Bank:** an area where sight is cut to 15 m, both ways, for 60 s (`rts/picture.lua` asks
    `Sp.fogged(x, z)`).
  - **Tide Surge:** knocks enemy troops in the area down (stunned 3 s) and puts out fires.
  - **Brine Mend:** heals one company, twice as much near water.
  - **Undertow:** enemy ships in the area sail at half speed for 15 s.
- **Storm-eye** is the storm tower's far-sight, with a radius of 250 m.

## Neutral places (`rts/neutral.lua`)

- **Market towns.** `N.market{ name, x, z }` builds walls, houses and a market square. It keeps
  `prices[res]` in gold per 10.
  - `N.trade(side, town, res, amount)` sells (amount > 0) or buys (amount < 0) at the town's
    price, and moves the price: selling lowers it, buying raises it, by 2% per 10.
  - The League trades at 10% better prices.
  - Every 60 s prices drift back towards their base.
- **Lighthouses.** `N.lighthouse{ name, x, z }` builds a tower with a stair and a lamp room.
- **Holding a place.** A place is held by the one side with units within 30 m of it and no enemy
  units there, for 10 s.
  - The holder of a market town gets 10 food, and every 10 s rumours: old sightings of all enemy
    units within 400 m.
  - The holder of a lighthouse has an eye of 300 m over the sea, and the lighthouse is a hub for
    its sending stones.
  - Event `rts_place_held`.

## The Reef War (`maps/reefholm.lua`)

- **The map:**
  - the sea with a reef archipelago;
  - the Crown's harbour town on the south shore (a keep, a harbour, a galleon and a caravel, a
    diving bell on the jetty);
  - the League's Tidehold anchored north of the reef, with a brigantine, two sloops and
    harpooneers on the islands;
  - the lighthouse on the reef, a market town on the east shore, pearl beds, and the sunken
    sky-barge between the reef and the town.
- **The end:** the Crown wins when the Tidehold sinks (the League's only headquarters), and loses
  if its keep falls.

## Tidesend (`maps/tidesend.lua`)

- **The map:** 2.4 km with a sea in the middle; the Crown, the Wildborn, the Gloam and the League
  in the four corners (the League on its shore); a market town and a lighthouse.
- **The match:** every side is played by the AI except the player's Crown.
