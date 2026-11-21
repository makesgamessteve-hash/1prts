# 12 · Roadmap

Each milestone becomes one or more **OpenSpec changes** (`openspec/changes/<id>/`) with a proposal,
spec deltas, a design and tasks. Every milestone ends in something playable on **all target
platforms** (Windows, Linux, macOS, web desktop, mobile web, WebXR), and every milestone must
pass the quality gates (≥ 95% unit-test coverage, automated game tests green; see
[architecture/quality-and-build.md](../architecture/quality-and-build.md)).

| Milestone | Codename | Goal | Planned OpenSpec change |
|---|---|---|---|
| **M0** | Concept | This concept, the architecture and the foundation spec | — (docs) |
| **M1** | Proving Ground | Engine foundation: walk around a test map and up a three-storey keep on every platform | `add-engine-foundation` |
| **M2** | First Command | RTS core: workers, gold and timber, soldiers, orders, pathfinding, the battlefield picture, field map, war table, sending stone, marking staff, horn, a basic AI | `add-command-core` |
| **M3** | Ride Out | Mounts for the commander and cavalry; crewed wagons and siege engines you can enter; nested moving reference frames; posts; deck plans; crew levels of detail | `add-mounts-and-engines` |
| **M4** | Hold and Harvest | Full economy (aether, food and folk, powder and bone, the six-tier system, upgrades), construction stages, fire, the Wildborn as the second faction, the behemoth, a base-building AI | `add-economy-and-second-faction` |
| **M5** | Sail and Wing | Water and sky: ships from cog to galleon and bonehulk, harbors, flyers (balloons, wyverns, rocs, skyships), ship-scale interiors and damage control | `add-water-and-sky-theaters` |
| **M6** | Skyfall | The storm layer and the high sky: drifting floating citadels, waking and steering them, the aether core, drop-gates, far-sight, sky galleons and elder drakes | `add-high-sky-citadels` |
| **M7** | Hollow Roads | The tunnels and deep caverns; the Gloam Courts as the third faction (stone and glowstone, tunnel wyrm, Sunken Throne); undermining | `add-underground-and-gloam-courts` |
| **M8** | Deep Water | Under the water; the Brine League as the fourth faction (pearls, Tidehold, diving bells, wind-barques); market towns and lighthouses | `add-underwater-and-brine-league` |
| **M9** | Every Screen | Polish for touch and WebXR (hands), mobile and headset performance, accessibility | `polish-touch-and-xr` |
| **M10** | Release 1.0 | Campaign, Custodians, editor tools, modding documentation, localization | several |

## M1 · Proving Ground (detailed in OpenSpec)

**Demo:** start the game on any platform and wake up on a small test map at dusk. Walk across
the meadow, open the oak door of a three-storey keep, climb the spiral stair or ride the
**hoist lift** up to the **war room**, and stand at a placeholder war table whose surface shows a
live rendered view. On a phone the touch controls appear on their own. In a headset you can enter
VR and do the same.

**Under the hood:** C engine core, SDL3 platform layer, OpenGL 3.3 / WebGL 2 retro renderer,
virtual file system with mod mounting, Lua 5.4 scripting with a sandbox and hot reload, action
input with touch and XR, basic 3D audio, headless mode, test framework with coverage gates,
automated game tests and CI on all platforms.

Full specification: [`openspec/changes/add-engine-foundation/`](../../openspec/changes/add-engine-foundation/proposal.md).

## M2 · First Command
- Entity simulation at a fixed tick rate; units with movement, weapons, sight and morale.
- Workers gather gold and timber and deliver it; trees can be felled; mines run out.
- Field navigation (grid plus hierarchical A\*), companies and formations.
- The battlefield picture: sightings with state and age; a basic sending-stone network (range,
  delay).
- Command tools: field map, war table (full RTS interface), sending-stone radial menu, spyglass
  and marking staff, horn signals.
- Crown roster for tier 1: serfs, men-at-arms, crossbowmen, outriders; holdfast, farmstead,
  barracks, sawmill, watchtower; a simple victory condition; a basic AI commander; the Training
  mission.
- **Exit:** a playable 1v1 skirmish on an S map against the AI (mirror match).

## M3 · Ride Out
- The commander on horseback; cavalry (knights, outriders).
- Nested reference frames: stand in a moving wagon, climb inside a siege tower as it is pushed,
  ride the mine's lift cage.
- Seats and posts, taking a post; boarding and leaving.
- Deck-plan compiler (geometry, collision, navigation, rooms) and the castle interior kit.
- Crew levels of detail (embodied, rostered, aggregated); room damage, first version.
- **Exit:** ride with your knights, climb a siege tower up to an enemy wall, walk the galleries of
  a gold mine.

## M4 · Hold and Harvest
- Aether, food and folk, powder and bone, the six-tier system (tier 5–6 content fills in with M5
  and M6), upgrades, construction stages, stone and timber buildings, fire and firefighting.
- Presence bonuses and succession.
- The Wildborn Clans as the second faction, including the behemoth as a moving war lodge;
  an AI that builds and expands.
- **Exit:** a full economic skirmish on an M map, Crown against Wildborn.

## M5 · Sail and Wing
- Water: aether skimmers and rafts, cogs, caravels, fireships, war galleons; longships, ram
  galleys, serpent riders, bonehulks; harbors and raft-yards; landings.
- Sky: observation balloons, wyverns, rocs, dwarven skyships; skyship masts and roost cliffs.
- Ship-scale interiors with damage control; chart rooms as command posts.
- **Exit:** the *Harbor Siege* scenario.

## M6 · Skyfall
- Drifting floating citadels on tracks over the map; waking and steering them; the weightless
  aether core; the throne hall's scrying floor; drop-gates and sky bombardment; far-sight.
- The storm layer: cloud deck, lightning, winds.
- Sky galleons and elder drakes (tier 6).
- **Exit:** the *Storm the Citadel* scenario, and a citadel phase in M/L skirmishes.

## M7 · Hollow Roads
- Tunnels and deep caverns as theaters: tunnel kit and cavern pieces, cells and portals for
  visibility and sound, digging tunnel routes, collapse and flooding.
- The Gloam Courts: delvers, stone and glowstone, the 21 units and 15 building types from
  [05](05-units-structures-interiors.md), echo pillars, the Sunken Throne and the tunnel wyrm's
  gondola as command posts, Glimmer-wright and Mycel keeper spells.
- Deep mine shafts (tier 4) so every faction can reach the caverns; glowstone grottos.
- **Exit:** the *Under the Hill* scenario, and four-way skirmishes with three factions.

## M8 · Deep Water
- Under the water as a theater: swimming and diving (breath charm), underwater fog and light,
  reefs, pearl beds, sunken wrecks.
- The Brine League: dockhands and pearl divers, pearls, the 21 units and 14 building types from
  [05](05-units-structures-interiors.md), the Tidehold and diving bells as command posts,
  wind-barques in the storm layer, Weatherwright and Tidecantor spells.
- Neutral market towns (trade, rumor network) and lighthouses.
- **Exit:** the *Reef War* scenario, and full four-faction skirmishes on L maps.

## M9 · Every Screen
- Touch interface usability pass; WebXR hand tracking; comfort options for riding and flying;
  performance work for mobile and standalone headsets; full accessibility pass.

## M10 · Release 1.0
- *The Skyfall Wars* campaign (prologue and acts in [15](15-story.md)); the Custodians;
  first-person map and scenario editor; modding guide and API reference; localization framework.
