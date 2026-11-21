# Proposal: First Command (M2), the RTS core

## Why

M1 proved the first-person engine; Harrowmark gave it a castle and a story, and the fun audit
showed what M1 cannot have: stakes. Flow and pacing stay low because nothing can be won or lost.
M2 turns the game into a real-time strategy game, played only through the commander's eyes:
workers, gold and timber, soldiers, orders that travel over sending stones, a picture of the
battlefield built from what your people see, and an opponent that plays back.

## What Changes

- **Navigation (engine):** a walkability grid built from the collision world and the terrain
  (cells, slope and clearance), A* paths with deterministic tie-breaking, smoothed by line of
  sight, and cells blocked and freed at run time as buildings rise and fall.
- **Agents (engine):** bulk movement for many units in C. An agent follows a path at its speed
  on the ground, keeps apart from its neighbours, turns to face where it goes, and reports
  arrival. Proximity queries return agents near a point in a stable order (for sight,
  targeting, gathering). Agents are simulation state: in the state hash and in saves.
- **The rules (scripts):**
  - Crown tier 1: serfs, men-at-arms (squads), crossbowmen (squads), outriders; holdfast,
    farmstead, barracks, sawmill, watchtower. All are definitions (`prts.def`), so mods change
    them.
  - Gold mines that run out and trees that are felled; serfs carry sacks and logs to the
    holdfast or a sawmill. Food sets the folk cap.
  - Construction by serfs at sites placed with the marking staff; training queues.
  - Combat with melee and ranged weapons and morale at a throttled think rate.
  - Companies with names, standing orders and orders: move, attack-move, hold, gather, build.
  - The sending-stone network: orders reach companies within range of a hub after a short delay;
    out of range they keep their last order.
  - The picture of the battlefield: sightings (seen, old) from what each side's people see; no
    global reveal.
  - A basic AI commander that builds an economy, trains an army and attacks.
  - Victory when the enemy holdfast falls; the *Training* mission and a 1v1 skirmish.
- **Command tools (first person):** the war table (the full RTS interface on the table in the
  holdfast's war room), the handheld field map, the sending-stone radial menu, the spyglass and
  marking staff, and the war horn.
- **A skirmish map**, *Ashford Vale* (1.6 km): two holdfasts, mines, forests and a ford.
- The fun audit learns to play a skirmish (W = winning against the AI), so flow and pacing get
  measured with stakes.

Engine (C): `nav/` and the agents, their Lua API, saves and hash. Scripts and data (Lua,
assets): everything else.

## Impact

- Windows, Linux, macOS, web desktop: the skirmish runs everywhere; CI plays an AI-versus-AI
  match headless and checks it ends with a winner.
- Mobile web: the war table and field map are touch-driven UI surfaces; the stone radial menu
  has a touch button.
- WebXR: the war table is used with the pointer ray (like the M1 notice board and winch).
- Saves: snapshot format 3 (agents, navigation changes).
- Specs: new capabilities `navigation`, `rts-core`, `command-tools`.

## Out of scope

- Mounts, cavalry riding with the commander, entering wagons (M3); the full 8 km S map (needs
  horses, M3). Ashford Vale is a small valley for M2.
- The economy beyond gold, timber, food and folk; tiers above 1; the Wildborn (M4).
- Presence and succession, messengers, wards, scrying and the other pouch tools (later).
- Multiplayer.
