# Design: fun audit, movement feel and goals

## The audit

- **The harness is a mod.** It lives in `tests/fun/mods/fun_eval` and is never shipped. The
  audit copies it into each run's user folder and enables it with `--mod`. It hooks the game
  from Lua (it wraps `prts.audio.play` and the subtitle functions, and listens to `use` events)
  and writes `FUN {json}` log lines stamped with simulation ticks, so the engine needed no
  telemetry API.
- **Bots drive the action layer** (`t._hold`, `t.look`, `t.face`), as a player does.
  - The human persona's limits (200 ms ± 35 ms perception, 6 rad/s turns, a 2° aim error) come
    from the skill.
  - Its habits are added to match players: it waits for the prompt before pressing, and slows
    to stop on a spot.
- **Routes are a hand-made graph of Harrowmark.** Each way carries the moves that cover it. When
  a way is blocked, the bot turns back to the nearest node, marks the way, and plans again. The
  audit reports blocked ways; that is how the stair headroom bug showed up.
- **Metrics are pure Python** (`tools/fun/metrics.py`) with their own tests.
  - Where the game has no threats or health, the measure is reported as n/a rather than made up.
  - Each sub-score is piecewise linear: 1 inside the skill's target, 0 at a stated limit.
- **Fair comparison:** the baseline was measured by building the previous commit in a separate
  worktree and running the final harness against it.

Alternatives considered:
- A C telemetry API: more code to test, and not needed.
- A real pathfinding navmesh for the bots: too much for one map. The route graph is small and
  checks the routes the level designer intends.

## Movement feel

- **Easing** moves the horizontal velocity towards the wanted one by at most `accel × dt` a tick:
  24 m/s² speeding up, 30 m/s² stopping, 8 m/s² in the air.
  - Running speed (3.5 m/s) is reached in 5 ticks; a stop takes 4.
  - Hitting a wall sets the velocity to what actually moved, so nothing builds up against the
    wall.
  - In XR the velocity is the wanted one at once: acceleration you don't cause yourself makes
    people sick.
- **Input buffer:** a jump press sets a counter of 5 ticks (its own tick included), which counts
  down while there is no ground. A press up to 120 ms before landing jumps on the first tick on
  the ground.
- **Coyote time:** leaving the ground without jumping leaves 4 ticks in which a jump still works.
  A press up to 100 ms after the edge jumps.
- **Hang time:** gravity × 0.6 while |vertical speed| < 1 m/s in the air. Air time goes from 25
  to 28 ticks, and the jump is 5 cm higher.
- **State:** the velocity, the buffer and the coyote counter are simulation state. They are in
  the state hash and in saves (snapshot format 2).

## The view's own motion

A spring for dips (landing kick 0.55 m/s per m/s of impact; jump 0.25 m/s), a head bob and a
shake. The bob is |sin| of a phase that advances by speed over a stride length; a footfall is
counted at each low point. The shake is trauma² times a sum of sines of a tick counter, at most
3 cm and 0.03 rad (2.5% of the 70° view).

- **Presentation only.** The offsets move the render camera; never the body, the use ray or the
  audio listener. They are not in the state hash or saves.
- **Deterministic all the same.** Everything is computed per tick from simulation state, so
  replays stay byte-identical.
- **Off when asked.** The camera motion setting scales the offsets. It is kept by the engine and
  given to the commander every tick, so spawning or loading doesn't reset it. XR forces it to 0.
- **Scripts react through `prts.commander.feel()`** (footfall count, landing speed, jump started)
  and add impulses with `prts.commander.impulse{ dip, shake }`. Footstep sounds live in Lua, as
  game logic does.
- **Cosmetic randomness** (pitch and volume of sounds) comes from a small stream in `game/feel.lua`,
  not the simulation's random numbers.

## Goals

- `story/progress.lua` is generic: places (a check of position) and orders (a check of state).
- A map's story module provides the lists: `story/harrowmark.lua` gives Harrowmark's six places
  and four orders.
- **When a card shows:** a place is announced when the commander gets there by moving. A jump of
  more than 15 m in one tick means the game put them there (spawn, save, test), and the place is
  recorded quietly. The longest VR teleport is 12 m.
- **Orders** are ticked off on the existing notice board (`board.done`). Their order is the
  board's order.

## The bake

`prts_cworld_occluded` used to collect every box under the bounds of the segment (most of the
castle for a 400 m sun ray), sort them, and then test each one. It now walks the BVH along the
ray, skipping nodes the ray misses (slab test on bounds grown by 1 mm), and stops at the first
hit. It keeps the same AABB filter on each box, so the set of boxes that can be hit, and so the
answer, is the same: `collide.bvh_matches_checking_every_box` compares it with brute force on
400 random rays, and the golden images are unchanged.

## Testable headless

Everything except the frames and the load time runs headless.
- **C unit tests:** the commander's easing, buffer, coyote, hang time and view motion; audio
  pitch; saves.
- **Lua unit tests:** footsteps, cards, progress, orders, turning, slams.
- **Scenarios:** feel, the castellan's orders, the war room stairs, talking.
- **The audit's survey** is headless too.
