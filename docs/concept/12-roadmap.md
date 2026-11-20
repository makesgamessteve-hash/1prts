# 12 · Roadmap

Each milestone becomes one or more **OpenSpec changes** (`openspec/changes/<id>/`) with a proposal,
spec deltas, a design and tasks. Every milestone ends in something playable on **all target
platforms** (Windows, Linux, macOS, web desktop, mobile web, WebXR), and every milestone must
pass the quality gates (≥ 95% unit-test coverage, automated game tests green; see
[architecture/quality-and-build.md](../architecture/quality-and-build.md)).

| Milestone | Codename | Goal | Planned OpenSpec change |
|---|---|---|---|
| **M0** | Concept | This concept, the architecture and the foundation spec | — (docs) |
| **M1** | Proving Ground | Engine foundation: walk around a test map and into a multi-storey HQ on every platform | `add-engine-foundation` |
| **M2** | First Command | RTS core: units, orders, pathfinding, the tactical picture, slate, table, radio, designator, a basic AI | `add-command-core` |
| **M3** | Ride Along | Crewed, enterable vehicles; nested moving reference frames; stations; deck plans; crew levels of detail | `add-crewed-vehicles` |
| **M4** | Supply Lines | Economy, personnel, logistics, construction, power grid; second faction; a base-building AI | `add-economy-logistics` |
| **M5** | Combined Arms | Air and sea theaters: aircraft, ships (destroyer, carrier), airfields, shipyards, damage control | `add-air-naval-theaters` |
| **M6** | High Guard | Orbit theater: spaceport, shuttle ascent, satellites, orbital station with spin gravity and zero-g | `add-orbital-theater` |
| **M7** | Every Screen | Polish for touch and WebXR (hands), mobile and headset performance, accessibility | `polish-touch-and-xr` |
| **M8** | Release 1.0 | Campaign, Custodians, editor tools, modding documentation, localization | several |

## M1 · Proving Ground (detailed in OpenSpec)

**Demo:** start the game on any platform and wake up on a small test map at dusk. Walk across
the terrain, open the door of a three-storey HQ building, take the stairs or the elevator to the
Command Center, and stand at a placeholder map table that shows a live render-to-texture
feed. On a phone the touch controls appear on their own. In a headset you can enter VR and do
the same.

**Under the hood:** C engine core, SDL3 platform layer, OpenGL 3.3 / WebGL 2 retro renderer,
virtual file system with mod mounting, Lua 5.4 scripting with a sandbox and hot reload, action
input with touch and XR, basic 3D audio, headless mode, test framework with coverage gates,
automated game tests and CI on all platforms.

Full specification: [`openspec/changes/add-engine-foundation/`](../../openspec/changes/add-engine-foundation/proposal.md).

## M2 · First Command
- Entity simulation at a fixed tick rate; units with mobility, weapons and sensors.
- Terrain navigation (grid plus hierarchical A\*), groups and formations.
- Tactical picture: contacts with confidence and age; basic comms network (range, delay).
- Command tools: slate, map table (full RTS interface), radio radial menu, binoculars and
  designator.
- 6 ground units, HQ, simple victory condition; a basic AI commander; Training mission.
- **Exit:** a playable 1v1 skirmish on an S map against the AI.

## M3 · Ride Along
- Nested reference frames: walk inside moving vehicles; vehicles carrying vehicles.
- Seats, crew stations, taking a station; embark and disembark.
- Deck-plan compiler (geometry, collision, navigation, compartments) and an interior kit.
- Crew levels of detail (embodied, rostered, aggregated); compartment damage, first version.
- **Exit:** ride in an APC, climb into a tank turret, walk the decks of a Command Crawler while it moves.

## M4 · Supply Lines
- Resources, depots, supply routes, power grid, construction stages, factories with assembly lines.
- Personnel recruitment and training; presence bonuses; succession.
- Second faction (Free Hollows Compact); an AI that builds and expands bases.
- **Exit:** a full economic skirmish on an M map.

## M5 · Combined Arms
- Air: helicopters, VTOLs, jets, AWACS, airlifters; airfields; air sensors and air defence.
- Sea: patrol boats, landing ships, destroyers, submarines, carrier; ports and shipyards.
- Ship-scale interiors and damage control; CIC command posts; launch and recovery.
- **Exit:** the *Carrier Defence* scenario.

## M6 · High Guard
- Orbit theater; shuttle ascent and descent; satellites with pass windows.
- Orbital station: zero-g hub and spin-gravity ring; Orbital Command Deck; orbital drops and strikes.
- **Exit:** the *Station Siege* scenario, and an orbital phase in M/L skirmishes.

## M7 · Every Screen
- Touch interface usability pass; WebXR hand tracking; comfort options; performance work
  for mobile and standalone headsets; full accessibility pass.

## M8 · Release 1.0
- *The Veyra Campaign*; the Custodians; first-person map and scenario editor;
  modding guide and API reference; localization framework.
