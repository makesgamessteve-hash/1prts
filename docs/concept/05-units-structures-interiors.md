# 05 · Units, Structures & Interiors

All sizes are in meters. Crew is the number needed for full effectiveness. "Interior" says what you
can walk into. This is the base roster for the reference faction. Both factions'
variations are covered in [08](08-setting-and-factions.md). All values are starting points, set
in Lua definitions, and will be tuned.

## 1. Roster

### Infantry and teams (sprites)
| Unit | Size | Crew | Role |
|---|---|---|---|
| Rifle squad | 8 people | 8 | Hold ground, capture, boarding |
| Weapons team | 4 | 4 | Anti-armor and anti-air missiles |
| Engineer team | 4 | 4 | Build, repair, demolish, clear mines |
| Recon team | 3 | 3 | Spot, designate, infiltrate |
| Medic team | 2 | 2 | Treat casualties (including the commander) |
| Damage control party | 4–6 | — | Ship and station internal crews; not built separately |

### Ground vehicles
| Unit | Length | Crew (+passengers) | Interior | Role |
|---|---|---|---|---|
| Scout buggy | 4.5 | 2 | Open cabin | Fast recon |
| Utility truck | 7 | 1 (+10) | Cab, cargo bed | Transport, towing |
| APC "Mule" | 8 | 3 (+8) | Driver, turret, troop bay | Infantry transport |
| MBT "Bastion" | 10 | 4 | Driver's compartment, turret basket | Main battle tank |
| SP artillery | 12 | 5 | Fighting compartment | Indirect fire |
| Mobile AA | 9 | 3 | Turret, radar console | Air defence |
| EW truck | 9 | 3 | Equipment shelter | Jamming, ELINT |
| Mobile Command Vehicle | 14 | 6 (+2) | Command shelter with a small table | Mobile command post, relay |
| Heavy hauler | 16 | 2 | Cab, cargo deck | Logistics, tank transport |
| Command Crawler | 60 × 24 × 18, 4 decks | 120 | Full multi-deck interior | Mobile HQ (Compact signature) |

### Aircraft
| Unit | Length / span | Crew (+pax) | Interior | Role |
|---|---|---|---|---|
| Recon drone | 6 span | 0 (remote) | — | Recon; flown from a ground station |
| Attack helicopter | 17 | 2 | Tandem cockpit | Anti-armor |
| Transport VTOL | 25 | 4 (+24 or 1 light vehicle) | Cockpit, cargo bay with ramp | Air assault, liaison flights |
| Strike fighter | 18 | 1–2 | Cockpit | Air superiority, strike |
| Airborne C2 (AWACS) | 45 | 12 | Cockpit, operator cabin with a table | Flying command post |
| Heavy airlifter | 60 | 6 (+2 MBTs or 120 troops) | Flight deck, cargo hold | Strategic lift |

### Naval
| Unit | Length | Crew | Interior | Role |
|---|---|---|---|---|
| Patrol boat | 25 | 12 | Wheelhouse, small cabin | Coastal patrol |
| Landing ship | 120 | 60 (+300 troops / 12 vehicles) | Bridge, vehicle deck, well deck | Amphibious assault |
| Submarine | 110 | 130 | Control room, sonar, torpedo room, berthing | Stealth, anti-ship |
| Destroyer | 155 | 280 | Bridge, CIC, radar room, engine rooms, magazines, mess, berthing | Escort, air defence |
| Carrier | 330 | 4,800 | Flight deck, hangar, island and bridge, CIC, ~40 hero rooms, ~1,500 generated compartments | Air power at sea, naval hub |

### Space
| Unit | Length | Crew (+pax) | Interior | Role |
|---|---|---|---|---|
| Recon / relay satellite | 12 | 0 | — | Imaging passes, comms relay |
| Orbital shuttle | 37 | 5 (+20 or cargo) | Flight deck, cabin, cargo bay, airlock | Ascent and descent, logistics |
| Drop carrier | 70 | 20 (+4 drop pods) | Bridge, pod bay | Orbital assault |
| Gunship | 90 | 30 | Bridge, weapons bay, engineering | Orbital combat |
| Orbital cruiser | 600 | 900 | Bridge, CIC, hangar, spinal weapon gallery, drive section | Capital ship |
| Orbital station "High Guard" | 1,400 ring diameter | 2,500 | Zero-g hub, spokes, spin-gravity ring with decks, docks, Orbital Command Deck | Orbital base, shipyard, command post |

### Structures
| Structure | Footprint | Staff | Interior | Function |
|---|---|---|---|---|
| HQ bunker | 60 × 60, 3 floors + basement | 80 | Command Center, briefing room, comms, medical, quarters | Command post, succession anchor |
| Barracks & Academy | 40 × 80 | 40 | Dormitories, classrooms, simulators | Recruit and train personnel |
| Power plant | 60 × 40 | 30 | Turbine hall, control room | Power grid |
| Mine / extractor | 30 × 30 + shaft | 20 | Headframe, control hut, shaft elevator | Ore |
| Refinery | 80 × 60 | 40 | Process hall, control room | Ore → alloy, crude → fuel |
| Depot | 50 × 50 | 15 | Warehouse, loading docks | Storage, resupply point |
| Vehicle factory | 220 × 60 | 150 | Assembly line, foreman's office | Ground vehicles |
| Munitions plant | 80 × 40 | 60 | Production line, bunkered storage | Munitions |
| Airfield | 2,200 runway + hangars | 200 | Tower, hangars, briefing | Fixed-wing aircraft |
| Helipad / VTOL base | 60 × 60 | 30 | Hangar, ops room | Rotorcraft |
| Shipyard (dry dock) | 400 × 80 | 400 | Dock floor, cranes, workshops | Ships |
| Comms tower | 60 tall | 2 | Equipment hut, ladder, platforms | Relay node |
| Radar station | 30 × 30, 40 tall | 10 | Operations room | Air and surface picture |
| Spaceport | Launch pad + 100 m gantry + hangars | 300 | Control center, gantry elevators | Launch and landing |
| Defences | Bunkers, gun emplacements, SAM sites | 2–10 | Fighting positions | Defence |

## 2. Interiors

### Interior classes
| Class | Examples | How it is made |
|---|---|---|
| Cab | Buggy, truck, helicopter cockpit | Hand-built low-poly model with seat markers |
| Compartment | Tank, APC, artillery | Hand-built model with stations |
| Multi-room | VTOL, MCV, AWACS, small buildings | Hand-built model or a deck plan |
| Ship-scale | Destroyer and larger, factories, HQ, stations | **Deck plans** plus **hero rooms** |

### Deck plans
Nobody can hand-build 1,500 carrier compartments. Large interiors are described as **deck
plans**: grid-based layouts (2 m cells, 2.6–3 m decks). Each character or tile in the plan maps
to a piece of a retro **modular kit** (walls, hatches, consoles, bunks, pipes, ladders). The
engine turns a deck plan into geometry, collision, crew navigation and compartment data.
**Hero rooms** (bridges, CICs, command centers, hangars, engine rooms) are hand-built and placed
into the plan. Deck plans are text files, so they are easy to mod (see [11](11-modding.md)).

### Why interiors matter (Pillar P4)
- **Command posts** are interiors.
- **Compartments are the damage model.** Each unit has **subsystems** (propulsion, weapons,
  sensors, comms, command, flight ops, life support), and each subsystem lives in compartments
  and stations. Flood the radar room and the radar goes down.
- **Production** happens on factory floors that you can see and reconfigure.
- **Crew stations** decide capability. An unmanned gunnery station means that gun does not fire.
- **Boarding** (later milestone): infantry enter enemy structures and ships and capture them
  room by room by taking the command room.

## 3. Crew

### Personnel records
Every crew member is a **roster record**: name, role, rank, skill (0–100), experience, morale and
fatigue. Crews come from the Barracks & Academy, as described in
[06](06-economy-and-logistics.md). Surviving crew keep their experience.

### Roles
Commander, driver, gunner, loader, pilot, helmsman, radar operator, sonar operator,
comms operator, engineer, damage control, medic, flight-deck crew, cook, quartermaster…
Roles decide which stations a crew member can take and how well they do it.

### Crew levels of detail
Thousands of people cannot all be simulated in full at once, so crew use three levels of detail:

| Level | When | Simulated as |
|---|---|---|
| **Embodied** | The commander is inside or close to the unit | Visible sprites that walk the deck plan, man stations, run to battle stations and fight fires |
| **Rostered** | The unit is in normal play elsewhere | Records assigned to stations and compartments, with no positions |
| **Aggregated** | Distant or idle formations | Numbers per role, plus average skill and morale |

Switching levels is deterministic. When you board a carrier, the crew appear at the stations
the roster says they are on, depending on shift and alert state. You will always find the chief
engineer in the engine room during battle stations.

### Damage control loop (ships, stations, big structures)
Hit → compartment event (fire, flooding, breach, power loss) → the alarm sounds → damage control
parties are sent along the deck plan → repairs take time and skill → subsystems come back online.
If the commander is present, this is faster (presence), and you can **prioritize** compartments
from the bridge or CIC.

## 4. Production is physical

- Vehicles move through **assembly stages** on the factory line and leave through the factory
  doors.
- Aircraft are rolled out of hangars. Ships are floated out of dry docks. Spacecraft are rolled
  to the pad, stacked and launched.
- Every product needs **crew** from the personnel pool before it can leave the factory.
- You can enter units that are only partly built and watch the work.
