# 07 · Modes & Match Flow

## 1. Modes

| Mode | Description | Milestone |
|---|---|---|
| **Proving Ground** | Sandbox test map for walking, entering buildings and trying out tools | M1 |
| **Training** | A guided tutorial for first-person command: slate, radio, table, designator | M2 |
| **Skirmish** | 1v1 up to 4 players (free-for-all or teams) against AI on generated or authored maps | M2+ |
| **Scenarios** | Hand-made set pieces: *Carrier Defence*, *Bridgehead*, *Station Siege* | M5+ |
| **Campaign** | *The Veyra Campaign*, a story-driven series of missions | M8 |
| **Multiplayer** | Versus and co-op command (see §5) | Post-1.0 |

## 2. Match setup (skirmish)

- **Map**: size S/M/L, theaters on or off (sea, air, orbit), climate and weather pattern.
- **Start**: *Bunker* (HQ plus a small force), *Landing* (Command Crawler or landing ship only)
  or *Established* (a small base).
- **Rules**: number of successors, logistics (physical or simplified), command pause on/off,
  transit time compression on/off, AI difficulty and personality.
- **Victory conditions** (one or more):
  - *Decapitation*: the enemy has used up all successors or has lost all succession anchors.
  - *Domination*: hold a majority of control zones for N minutes.
  - *Score*: highest score at a time limit (economy, losses, objectives).
  - *Scripted*: a scenario's own Lua conditions.

## 3. Time

- The simulation runs in **real time**.
- **Command pause** (single player, optional rule): while you are engaged with a map table, you
  may pause the world and plan. This is off by default for "Iron Commander" difficulty.
- **Transit time compression** (single player, optional rule): ×2/×4 while in transit with no
  hostile contacts nearby.
- **Saving**: anywhere in single player. **Replays**: recorded from the commander's point of
  view. Developer builds can also replay with an observer camera.

## 4. Campaign outline: *The Veyra Campaign*

Five acts, about 15 missions. The same story can be played from either faction's side, with
different missions.

1. **Charter's End**: the colonial charter collapses. A ground-only act that teaches command
   from a single bunker and one MCV.
2. **The Canyon War**: mobile warfare, supply lines and the first Command Crawler duels.
3. **Coastline**: naval and air theaters open up: amphibious landings and carrier operations.
4. **High Guard**: the race for orbit: spaceports, the first station, satellite warfare.
5. **The Custodians Wake**: the war damages the pre-colonial terraforming machines, which
   wake up. A third side appears (voxel-based; see [08](08-setting-and-factions.md)).

Missions are Lua scripts, so the campaign is a mod like any other.

## 5. Multiplayer outlook (post-1.0)

The architecture keeps multiplayer possible, but it is not part of the first release.
- **Versus**: 1v1 up to 4 players, each a physical commander. Hunting the enemy commander becomes
  a true duel.
- **Co-op command**: several commanders share one faction (for example ground, naval and orbital
  commanders) and share a tactical picture.
- **Officers mode**: other players join as subordinate officers in first person, such as a
  platoon leader or a ship captain, and receive the commander's orders. Players are not forced
  to obey.

## 6. Session lengths

| Mode | Target length |
|---|---|
| Skirmish S | 30–45 min |
| Skirmish M | 45–90 min |
| Campaign mission | 30–75 min, with saves |
| Scenario | 15–40 min |
