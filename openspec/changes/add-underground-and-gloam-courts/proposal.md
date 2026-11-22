# Proposal: Hollow Roads (M7), the tunnels, the deep caverns and the Gloam Courts

## Why

Two of the concept's eight theaters lie under the field (concept 04 §2):
- the **tunnels**, mine galleries and the Gloam's tunnel roads;
- the **deep caverns**, with fungus forests, glowstone grottos and the Gloam courts.

Their people, the Durrow of the **Gloam Courts**, are the third faction (concepts 05 §3, 06, 08,
14). Story act VI (concept 15) opens their sealed gates: Caldrith's broken chains run down into
the deep caverns. *Under the Hill*. M7 brings the underground as a place the commander walks, the
Gloam Courts, that scenario and four-way skirmishes with three factions.

## What Changes

- **Engine (C):** the ground can be dug.
  - Terrain **holes** can be given when the terrain is made, or cut later with `prts.scene.hole`.
    Inside a hole the terrain is neither drawn nor solid.
  - More than 3 m under the terrain's surface, the terrain no longer collides. Galleries built
    of boxes under the ground are walkable, and the navigation grid ignores them: units walk on
    the field.
- **Tunnels and caverns** (scripts, `rts/tunnels.lua`):
  - **The tunnel kit:** galleries (straight, level or sloped), mouths (ramps down through a hole
    in the ground) and caverns, with glowstone light. Each part is a cell, with portals where
    they meet, so visibility follows the tunnels.
  - **Tunnel roads:** a graph of nodes and galleries. Companies ordered through a tunnel leave
    the field at one mouth, travel unseen under the ground and come out at the other.
  - **Digging:** delvers dig new galleries and mouths, quickly; other workers dig slowly. Every
    metre gives the Gloam stone. Digging rumbles underfoot, the only warning on the surface.
  - **Collapse and flooding:**
    - a gallery can be collapsed (it is blocked, and those inside are hurt);
    - low galleries near water flood, and the commander wades.
  - **The dark:** underground the fog closes in, dark, and the day does not reach.
  - **Deep mine shafts (tier 4):** every faction can sink one, with a lift cage down to the
    caverns.
- **The Gloam Courts** (scripts, `rts/gloam.lua`), the third faction:
  - **Resources:** stone (quarries and digging) and glowstone (glowstone works over a grotto),
    with no timber.
  - **Units:**
    - delvers, basalt guard, deep pikes, slingers (their glowing shot keeps targets seen);
    - undercutters (they bring buildings down from below), the hushfoot (hard to see), echo
      hounds and carapace riders on cave beetles;
    - the olm brute and moth riders;
    - two casters, the **Glimmer-wright** and the **Mycel keeper**.
  - **Buildings:**
    - the headquarters, **Delve → Hollow Court → Sunken Throne**: a gatehouse on the surface,
      with its hall and war room down a stair shaft under the ground;
    - mycel gardens, guard hall, stonecutters' hall, glowstone works, beetle pens;
    - crystal sanctum (deep-sight), spore grotto, engine vault, wyrm hollow;
    - tunnel gates, glow towers and **echo pillars**, sending-stone relays that reach
      companies under the ground.
  - **The tunnel wyrm:** a beast that burrows under the map with troops in its harness gondola
    (with a small war table), and bursts up under buildings.
  - **Spells:**
    - Glowflare, Stonemeld and Crystal Sight for the Glimmer-wright;
    - Spore Cloud, Fungal Mend and Echo Call for the Mycel keeper.
  - **AI:** the AI plays the Gloam like the other two factions.
- **Under the Hill** (scenario map *Underhill*):
  - The Gloam open their sealed gates under the hills of the Merewatch, where Caldrith's broken
    chain runs down into a glowstone cavern.
  - The Crown holds the hill above, with a deep shaft sunk into the caverns.
  - The Crown wins when the Hollow Court falls, and loses if its keep falls.
- **Four-way skirmishes:**
  - a match can have up to four sides, every side against every other;
  - the AI picks its foe;
  - the last side standing wins.

  *Hollowfell* is a large four-corner map: the Crown, the Wildborn, the Gloam and a rival Crown
  host.

## Impact

- **Engine:** terrain holes and the underground rule, in `scene` and `phys`. The nav grid
  ignores surfaces under the terrain. Saves: a hole cut at run time is entity data, and the map
  cuts it again on load.
- **All platforms.** The underground reuses cells and portals (M1), moving frames and deck plans
  (M3), and the sending-stone rules (M2).
- **Tests:** a game test per spec scenario; C tests for holes and the underground rule;
  coverage gates.
