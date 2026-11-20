# 04 · World, Scale & Theaters

## 1. Scale rules

- **1 world unit = 1 meter.** No exceptions.
- People are 1.6–1.9 m tall. Doors are 0.9 × 2.1 m, ship hatches 0.7 × 1.6 m, airlocks 1.2 × 2.0 m.
- Building storeys are 3.0–4.0 m (industrial halls 12–20 m). Ship decks are 2.6–3.0 m.
  Station ring decks are 3.0 m.
- Vehicles, ships, aircraft and spacecraft use believable real-world sizes (see
  [05](05-units-structures-interiors.md)).
- **Distances are compressed, sizes and speeds are not.** Bases sit closer together than in a
  real campaign so that a match fits into 45–90 minutes. A tank is still 10 m long and still does
  60 km/h.

## 2. Theaters

A match is played over up to four connected theaters.

| Theater | Space | Content |
|---|---|---|
| **Ground** | The map region (see sizes below) | Terrain, rivers, forests, settlements, roads, bases |
| **Sea** | The coastal and open-water part of the region | Ports, shipyards, fleets, submarines, amphibious landings |
| **Air** | Above the region, up to about 20 km altitude | Helicopters, VTOLs, jets, airlifters, AWACS, drones |
| **Orbit** | A separate connected space scene above the region | Satellites, shuttles, gunships, cruisers, orbital stations |

Ground, sea and air share one continuous world. Orbit is its own scene, linked through
**spaceports** (launch and landing) and through **orbit-to-ground effects**: recon passes, comms
relay, drop pods and orbital strikes.

### Map sizes

| Size | Area | Typical use | Platforms |
|---|---|---|---|
| S | 8 × 8 km | 1v1, ground and air | All, including mobile web |
| M | 16 × 16 km | 1v1 and 2v2 with sea | Desktop, web desktop, XR |
| L | 32 × 32 km | 4 players, full combined arms | Desktop |

### Travel times (why the commander rides)

| Mode | Speed | Crossing 8 km |
|---|---|---|
| On foot (run) | 12 km/h | 40 min |
| Truck / MCV | 70 km/h | 7 min |
| Helicopter / VTOL | 250 km/h | 2 min |
| Jet | 900 km/h | 32 s |
| Destroyer | 55 km/h | 9 min |

## 3. Orbit

- The battlefield is on **Veyra**, a habitable moon of the gas giant **Orun** (see
  [08](08-setting-and-factions.md)). Orun fills a large part of the sky and is a major visual
  landmark.
- **Low orbit** passes over the region. **Satellites** have **pass windows**: they can image the
  region or relay comms only while overhead. Orbital periods are compressed for gameplay
  (default: one pass every ~12 minutes of match time).
- **High Guard** is a station-keeping position above the region where orbital stations and big
  ships sit. They are always in contact and always in reach of each other.
- **Ascent and descent** feel seamless from inside. You stay in the shuttle cabin and watch the
  sky darken through the windows. In the engine, the craft is handed from the air theater to
  the orbit theater at the boundary (100 km).
- Space movement uses **simplified Newtonian flight**: momentum and thrust, with no real orbital
  mechanics during tactical flight. Orbits are scheduled tracks.

## 4. Moving interiors and reference frames

Because big things move while people are inside them, the world is built from **nested
reference frames**:

- Standing on a carrier's flight deck at 30 knots, you move with the ship. Wind and spray are
  effects only.
- Standing in the cargo bay of a flying airlifter, you can walk around. On landing, the ramp
  lowers and you walk out onto the ground.
- In an elevator, you ride the car. In a rotating station ring, **spin gravity** pulls you
  "down" towards the outer hull. Climb a spoke and it fades. In the hub you float.
- **Vehicles inside vehicles**: a tank drives into an airlifter's cargo bay, which flies to a
  landing ship's deck, and the commander can be sitting inside the tank the whole time.

How this works technically: [architecture/simulation.md](../architecture/simulation.md#reference-frames).

## 5. Environment

- **Day/night cycle**: one game day is 60 minutes by default (adjustable). Night needs lights,
  flares and night vision. Orun's reflected light makes "Orun-lit nights" brighter than
  moonlight.
- **Weather**: clear, overcast, rain, fog, dust storm, snow (depending on map). Weather affects
  visual range, sensor performance, flight operations and the sea state.
- **Sea state** (0–6) affects naval speed, flight-deck operations and small boats.
- **Terrain** affects movement (roads, off-road, swamp, forest), line of sight for radio and
  eyes, and cover.

## 6. Destruction

- **Structures and vehicles** have damage states: *intact → damaged* (smoke and fire sprites,
  missing panels, systems down) *→ wrecked* (a wreck model that blocks paths and can be salvaged).
- **Interiors** take damage per **compartment**: fire, flooding, breaches, vacuum in space,
  power loss. Damage control crews respond (see [05](05-units-structures-interiors.md)).
- **Terrain** gets crater decals and scorch marks. Deformable voxel terrain is an optional later
  feature (see [09](09-art-and-audio-direction.md)).
