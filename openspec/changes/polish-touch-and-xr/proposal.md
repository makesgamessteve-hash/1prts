# Proposal: Every Screen (M9), touch, hands, comfort, performance and accessibility

## Why

The game runs on desktops, phones and headsets (concept 10), and the eight milestones so far
have added far more than the M1 controls were tuned for:
- swimming and diving;
- riding horses, ships, skyships, citadels and wyrms;
- war tables in four factions' colours.

M9 is a polish milestone: it makes the game comfortable and readable on every screen and for
every player (concept 10 §2–3).

## What Changes

- **Touch:**
  - the context buttons can be made larger and moved to the left hand;
  - they say what they do under the water (Rise, Dive) and on a horse (Dismount).
- **WebXR hands:**
  - with hand tracking and no controllers, a pinch is the trigger, and the hand's ray is the
    pointer;
  - a left-hand pinch walks (or aims a teleport). The game asks for hand tracking when it enters
    VR.
- **Comfort for riding and flying:**
  - in VR the comfort vignette also darkens the edges while the commander is carried (on a
    horse, a ship, a skyship, a citadel, a wyrm), with its own strength;
  - both are options.
- **Performance:**
  - a draw distance beyond which sprites and moving things are not drawn, and the fog closes in
    before it;
  - render counters for tests;
  - a performance setting (low, medium, high), which starts at low on phones and in headsets.
- **Accessibility:**
  - colour-blind-safe palettes for the war table and field map, with sigils whose shape tells
    friend from foe (rings for your own, squares for the enemy);
  - a high-contrast table mode;
  - larger map text;
  - crouch and run can be toggles instead of held;
  - a brightness boost under the ground and under the water;
  - game speed (single player).

Engine (C):
- `prts.render.settings{ draw_distance }` and `prts.render.stats()`;
- the XR options `ride_vignette` and `ride_strength`;
- `prts.input.toggle(action, on)`;
- `prts.time.scale([s])`.

Web glue: hand input sources.

## Impact

- All platforms. No change to the simulation: game speed changes how fast ticks run against the
  clock, not what a tick does. Determinism holds.
- Settings are saved with the others.
- Tests:
  - C tests for the new engine parts;
  - browser tests for hands;
  - a game test per spec scenario;
  - coverage gates.
