# 07 · Modes & Match Flow

## 1. Modes

| Mode | Description | Milestone |
|---|---|---|
| **Proving Ground** | Sandbox test map (Harrowmark, the Crown's proving fortress) for walking, climbing the keep, riding the lift, meeting the castle's people and trying out tools | M1 |
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

A prologue at Harrowmark and eight acts, each built on what one roadmap milestone adds; the story,
cast and acts are in [15 · Story](15-story.md). About 15 missions per side; the story can be
played from any of the four sides, with different missions.

0. **Prologue: Harrowmark** (M1): the new Marshal arrives at the Crown's proving fortress; the
   north has gone silent.
1. **The Silent Watch** (M2): first command from the war table; raiders at the ford.
2. **The Border Marches** (M3): ride north to the silent watchtowers; the first siege tower.
3. **Timber and Gold** (M4): the Wildborn gather under a behemoth and besiege Harrowmark.
4. **The Saltcoast** (M5): the siege is broken by river and sky; the *Harbor Siege*.
5. **Above the Clouds** (M6): the fallen citadel Caldrith rises; *Storm the Citadel*.
6. **The Hollow Roads** (M7): the Gloam Courts open their sealed gates; *Under the Hill*.
7. **Deep Water** (M8): the Brine League and the war under the waves; the *Reef War*.
8. **The Custodians Wake** (M10): waking the citadels also wakes their ancient guardians. A
   fifth, hostile side appears (voxel-built; see [08](08-setting-and-factions.md)).

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
