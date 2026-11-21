# 07 · Modes & Match Flow

## 1. Modes

| Mode | Description | Milestone |
|---|---|---|
| **Proving Ground** | Sandbox test map for walking, climbing the keep, riding the lift and trying out tools | M1 |
| **Training** | A guided tutorial for first-person command: field map, sending stone, war table, marking staff, horn | M2 |
| **Skirmish** | 1v1 up to 4 players (free-for-all or teams) against AI on generated or authored maps | M2+ |
| **Scenarios** | Hand-made set pieces: *Hold the Ford*, *Harbor Siege*, *Storm the Citadel*, *Under the Hill*, *Reef War* | M5+ |
| **Campaign** | *The Skyfall Wars*, a story-driven series of missions | M10 |
| **Multiplayer** | Versus and co-op command (see §5) | After 1.0 |

## 2. Match setup (skirmish)

- **Map**: size S/M/L, which of the eight theaters are on (for example water, under water, tunnels,
  deep caverns, sky, storm layer, high sky), climate and weather.
- **Factions**: any mix of the four playable factions, including mirror matches.
- **Start**: *Holdfast* (headquarters, a few workers, a squad; the Gloam start in a Delve, the
  Brine League in a Wharf hall), *Landfall* (only a ship full of settlers, or for the Gloam a
  tunnel wyrm breaking out of the ground; build from nothing) or *Established* (a small town).
- **Rules**: number of successors, field supplies on or off, command pause on or off, travel time
  compression on or off, AI difficulty and personality.
- **Victory conditions** (one or more):
  - *Decapitation*: the enemy has used up all successors or has lost all succession anchors.
  - *Conquest*: destroy or capture every enemy headquarters, behemoth and Tidehold.
  - *Dominion*: hold a majority of the shrines or villages for N minutes.
  - *Citadel*: wake and hold a floating citadel for N minutes (high-sky maps).
  - *Trade*: hold the market towns and earn a set amount of gold from them.
  - *Scripted*: a scenario's own Lua conditions.

## 3. Time

- The simulation runs in **real time**.
- **Command pause** (single player, optional rule): while you are at a war table, you may pause
  the world and plan. Off at the "Iron Crown" difficulty.
- **Travel time compression** (single player, optional rule): ×2/×4 while travelling with no
  enemy nearby.
- **Saving**: anywhere in single player. **Replays**: recorded from the commander's point of
  view. Developer builds can also replay with an observer camera.

## 4. Campaign outline: *The Skyfall Wars*

Five acts, about 15 missions per side. The story can be played from any of the four sides, with
different missions.

1. **The Border Marches**: the old truce breaks. A field-only act that teaches command from a
   single holdfast and one war tent.
2. **Timber and Gold**: expansion, raids on gatherers, the first sieges, the first behemoth hunt.
   The Gloam Courts break out from below when the mines dig too deep.
3. **The Saltcoast**: rivers and the sea open up: landings, fireships, the fight for the aether
   springs. The Brine League's fleets arrive, and the war goes under the waves.
4. **Above the Clouds**: skyships and drakes, and the race to reach the drifting citadels.
5. **The Custodians Wake**: waking the citadels also wakes their ancient guardians. A fifth, hostile
   side appears (voxel-built; see [08](08-setting-and-factions.md)).

Missions are Lua scripts, so the campaign is a mod like any other.

## 5. Multiplayer outlook (after 1.0)

The architecture keeps multiplayer possible, but it is not part of the first release.
- **Versus**: 1v1 up to 4 players (one of each faction, if you like), each a physical commander.
  Hunting the enemy commander becomes a real duel.
- **Co-op command**: several commanders share one side (for example one on the field, one at sea,
  one in the sky) and share the war table.
- **Captains mode**: other players join as captains in first person, such as a knight captain or
  a galleon's master, and receive the commander's orders. Players are not forced to obey.

## 6. Session lengths

| Mode | Target length |
|---|---|
| Skirmish S | 30–45 min |
| Skirmish M | 45–90 min |
| Campaign mission | 30–75 min, with saves |
| Scenario | 15–40 min |
