# 04 · World, Scale & Theaters

## 1. Scale rules

- **1 world unit = 1 meter.** No exceptions.
- Humans are 1.6–1.9 m tall, Ghorr 2.1–2.4 m, Kobb 1.1 m, ridge giants 6 m.
- Doors are 1.0 × 2.2 m, castle gates 4 × 5 m, ship hatches 0.8 × 1.4 m.
- Storeys are 3.0–4.0 m; great halls 8–14 m; mine galleries 2.5 m; ship decks 1.9–2.2 m (sailors duck).
- Ships, siege engines, beasts and flying creatures use believable sizes (see
  [05](05-units-structures-interiors.md)).
- **Distances are compressed, sizes and speeds are not.** Keeps sit closer together than in a
  real war so that a match fits into 45–90 minutes. A knight still gallops at about 40 km/h, and
  a galleon still sails at 15 km/h.

## 2. Theaters

A match is played over up to four layers of one continuous world.

| Theater | Space | Content |
|---|---|---|
| **Field** | The map region (see sizes below) | Hills, forests, rivers, fords, villages, roads, mines, keeps |
| **Water** | Rivers, lakes and the coast | Harbors, fleets, landings, serpent riders, aether springs in the shallows |
| **Sky** | Above the region, up to about 2 km | Balloons, wyverns, rocs, skyships, drakes |
| **High sky** | 3–6 km up, above the clouds | Floating citadels, sky galleons, elder drakes |

All four layers are **one world**, so there is no loading between them. The high sky is cold,
windy and hard to reach. Only skyships, rocs and drakes go that high, plus the lift-stones of a
woken citadel.

### Map sizes

| Size | Area | Typical use | Platforms |
|---|---|---|---|
| S | 8 × 8 km | 1v1, field and sky | All, including mobile web |
| M | 16 × 16 km | 1v1 and 2v2 with water | Desktop, web desktop, XR |
| L | 32 × 32 km | 4 players, every theater | Desktop |

### Travel times (why the commander rides)

| Mode | Speed | Crossing 8 km |
|---|---|---|
| On foot (run) | 12 km/h | 40 min |
| Horseback (gallop, with rests) | 30 km/h | 16 min |
| Wagon on a road | 10 km/h | 48 min |
| Wyvern | 90 km/h | 5 min |
| Skyship | 50 km/h | 10 min |
| Galleon (good wind) | 15 km/h | 32 min |

## 3. The high sky and the floating citadels

- Centuries ago the High Imperium ruled from **floating citadels** held up by great
  **aetherstones**. When the Imperium fell (the **Skyfall**), most citadels crashed. A few still
  **drift**, dark and silent, high above the land (see [08](08-setting-and-factions.md)).
- Each drifting citadel follows a **slow, predictable track** over the map. Its passes over
  key areas work like spy-satellite passes in a modern game: when a citadel passes overhead it
  can watch, drop troops or bombard; at other times it is out of reach.
- A side that **wakes** a citadel (by bringing aether and mages to its core) can **steer** it
  slowly and use its throne hall as a command post. Waking and holding a citadel is the late-game
  goal on large maps.
- Flying up is seamless. You stand on the skyship's deck as the ground falls away, pass through
  the cloud layer and see the citadel's silhouette grow until you dock.

## 4. Moving interiors and reference frames

Big things move while people are inside them, so the world is built from **nested
reference frames**:

- Walking the deck of a galleon as it rolls in the swell, you move with the ship.
- Climbing the ladders inside a siege tower as it is pushed toward the wall, you stay inside it.
- Riding a lift cage down the mine shaft, you go down with the cage.
- Walking among the huts on a behemoth's back, you sway with every step the beast takes.
- Standing in a roc's basket or a skyship's gondola, you fly with it.
- In a citadel's **aether core**, you float.
- **Things inside things**: a catapult is loaded onto a galleon, the galleon lands at a beach,
  and the commander was below decks the whole time.

How this works technically: [architecture/simulation.md](../architecture/simulation.md#3-reference-frames).

## 5. Environment

- **Day/night cycle**: one game day is 60 minutes by default (adjustable). Nights need torches,
  lanterns and watch fires. Veyra has **two moons**, the pale Ilen and the red Corvane. A night
  with both full is almost as bright as dusk. A night with neither is truly dark.
- **Weather**: clear, overcast, rain, fog, storm, snow (depending on map). Weather affects sight,
  archery, sailing, flying and fire.
- **Wind**: direction and strength affect ships, skyships, arrows and how fire spreads.
- **Terrain** affects movement (roads, open ground, mud, forest, marsh), line of sight for eyes
  and sending stones, and cover.
- **Forests are a resource and a battlefield.** Trees are cut down for timber and do not grow
  back during a match. Forests block sight and slow horses. Forests can burn.

## 6. Destruction and fire

- **Buildings, ships and engines** have damage states: *intact → damaged* (smoke, flames,
  missing timbers, posts out of action) *→ ruined* (a wreck that blocks paths and can be
  salvaged for timber and gold).
- **Fire** is a real threat. Wooden buildings, ships, palisades and forests catch fire from fire
  arrows, firepots, dragon breath and spells. Fire spreads with the wind. Workers and crews put
  it out with buckets and sand. Stone does not burn, which is why stone keeps cost more.
- **Interiors** take damage **room by room**: fire, flooding (ships), collapse, smoke. Crews
  respond (see [05](05-units-structures-interiors.md)).
- **Terrain** gets scorch marks and craters. Deformable voxel terrain (dug trenches, collapsed
  mine tunnels) is an optional later feature (see [09](09-art-and-audio-direction.md)).
