# Proposal: A fun audit, and what it found (feel, goals, a blocked stair, load time)

## Why

The game-fun-evaluator skill (`.claude/skills/game-fun-evaluator/`) turns "is it fun?" into
measurements. Bots with human limits play the game, probes time its answers, and six pillar
scores combine into one number. Applied to Harrowmark it scored **51.5 / 100**: "a functional
prototype that lacks spark". The causes were concrete:

- 62% of jumps pressed with human timing were dropped: no input buffer, no coyote time.
- Motion started and stopped at full speed in one tick (a boxcar). Gravity was flat at the top
  of a jump.
- Running, jumping, landing, the lift arriving and the portcullis falling gave no feedback at
  all: no sound, no camera motion.
- The keep's stairs to the war room could not be climbed standing up. The floor over the upper
  landing left 1.7 m of headroom, so the stairs were a dead end next to the lift.
- After clicking the map, 4.1 s passed before the commander could move (sun bake).
- Between goals the game said nothing for up to 140 s. Reaching a part of the castle or doing
  one of the castellan's orders went unremarked.

## What Changes

- **A fun audit** (`tools/fun/audit.py`, `tests/fun/`):
  - A mod that is never shipped: telemetry, four bot personas (novice, human proxy, oracle,
    spam), Harrowmark's route graph, and feel and fairness probes.
  - A windowed pass that measures load time and the text contrast of overlays.
  - The skill's report, with a before/after comparison.
  - A CTest survey has the oracle walk every way of the route graph there and back, and fails on
    a blocked way.
- **Movement feel (engine):**
  - The body eases in over 4 ticks and stops over 4; in VR it stays at once, for comfort.
  - A jump press is kept for 5 ticks until there is ground (input buffer).
  - A jump still works 4 ticks after walking off an edge (coyote time).
  - Gravity eases near the top of a jump.
- **The view's own motion (engine, presentation only):**
  - A head bob with a footfall counter, a landing dip and a deterministic shake.
  - Scaled by a new "Camera motion" game option (0..1). Never in VR.
  - Not part of the simulation, the state hash or saves.
- **Feedback (scripts and sounds):**
  - Footsteps on footfalls, with slight pitch and volume variation.
  - A jump rustle, and a landing thud that is lower and louder the harder you land.
  - The portcullis slams and shakes the passage; the lift settles with a thud you feel when
    riding it.
  - People at their posts turn to whoever talks to them, with a murmur, then turn back.
  - The winch and lever prompts say what they will do.
  - Small props are picked a little beyond their edges.
- **Goals (scripts):**
  - Title cards when the Marshal first reaches each part of Harrowmark.
  - The castellan's four orders are tracked and announced, reported by the garrison and ticked
    off on the board: bar the King's Gate, man the wall-walks, count the arrows at a new arrow
    store in the barracks, watch the north from the Raven Tower. All four ring the dusk bell.
- **Fixes:**
  - The keep's upper landing gets standing headroom.
  - The sun bake walks the collision tree along each ray and stops at the first hit. Same
    result, load time 4.1 s → 0.9 s.
  - Saves store the commander's velocity, jump buffer and coyote time (snapshot format 2).
- **Audio:** sounds can be played at a pitch (0.5–2).

Engine (C): commander easing, buffer, coyote, hang time, view motion and its API, the bake's ray
walk, the snapshot fields, audio pitch. Scripts and data (Lua, assets): `game/feel.lua`,
`ui/banner.lua`, `story/progress.lua`, the Harrowmark places and orders, NPC turning, the props'
sounds and prompts, the arrow store, the stair slab, five generated sounds, the camera motion
option. Tools: the fun audit and its tests.

## Impact

- **Windows, Linux, macOS, web desktop:** the new feel and goals behave the same everywhere. CI
  runs the new scenarios headless and the fun audit's survey. Loading is faster on every
  platform; most of all on the web, where the bake runs single-threaded in WebAssembly.
- **Mobile web:** the same; the camera motion option is in the options menu.
- **WebXR:** the view's own motion is off, and walking starts and stops at once (comfort).
  Footsteps and other sounds still play.
- **Saves:** format 1 saves are refused with the usual "format not supported" message.
- **Specs:** additions to `commander-locomotion`, `storytelling`, `castle-content`,
  `audio-playback` and `automated-testing`.

## Out of scope

- Challenge and failure (raids, a dusk deadline with consequences, enemies). The audit's flow
  and pace pillars stay where they were until the skirmish milestones bring stakes.
- Particles, hitstop and the other remedies for combat games.
- Giving jump and crouch a use on Harrowmark's routes.
- Voice acting: the talk murmur is a placeholder.
