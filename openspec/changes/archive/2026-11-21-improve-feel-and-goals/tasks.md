# Tasks: a fun audit, and what it found

## 1. The audit

- [x] 1.1 Install the game-fun-evaluator skill (`.claude/skills/game-fun-evaluator/`, the formulas protected from the skill loader's argument substitution) and point `CLAUDE.md` at it; verify the skill loads with its formulas intact
- [x] 1.2 Write the audit mod (telemetry, personas, route graph, goals, probes, frame pass) and `tools/fun/audit.py` with `tools/fun/metrics.py`; verify `python3 -I tools/tests/test_fun_metrics.py` and a 20-episode run (spec *Auditing a build*)
- [x] 1.3 Add the audit's survey to CTest (`fun_survey`: the oracle walks every way of the route graph there and back); verify it passes, and fails naming `stair_foot - war_stair` on the previous commit (spec *A way through the map is broken*)
- [x] 1.4 Measure the baseline on the previous commit (a separate worktree, the final harness) and keep the reports in `docs/fun-audit/`

## 2. Movement feel and the view's motion

- [x] 2.1 Ease the commander's speed, add the jump buffer, coyote time and hang time (not in XR), and save the new state (snapshot format 2); verify `commander.movement_feel_buffer_coyote_and_easing`, `commander.walking_speed_and_crouch` and the save tests (spec *Speeding up and slowing down*, *Jump pressed just before landing*, *Jump just after the edge*, *Hang time*, *Feel stays out of VR*)
- [x] 2.2 Add the view's own motion (bob with footfalls, dips, shake; scaled by the camera motion setting; never in XR) with `prts.commander.feel`, `impulse` and `set_feel`, and the camera motion option; verify the commander unit test and `commander_feel.lua` (spec *Camera motion setting*)
- [x] 2.3 Play footsteps, jumps and landings from Lua, make the portcullis slam and the lift settle, add pitch to `prts.audio.play` and five generated sounds with credits; verify `audio.pitch_changes_the_playback_rate`, `scripts/tests/test_feel.lua` and `commander_feel.lua` (spec *Pitch changes the playback rate*, *Footsteps and landings*, *Heavy things nearby*)

## 3. Goals and people

- [x] 3.1 Add title cards for places and the castellan's orders (with the arrow store and the dusk bell); verify `test_feel.lua` and `castellan_orders.lua` (spec *Coming through the King's Gate*, *Doing the castellan's orders*)
- [x] 3.2 Turn people at their posts to whoever talks to them (the short way round) and back; verify `test_feel.lua` and `npc_talk.lua` (spec *Talking to a guard turns them*)
- [x] 3.3 Say what the winch and the lever will do, and pick small props a little beyond their edges; verify `test_hud.lua` and the audit's use-box ratios

## 4. Fixes

- [x] 4.1 Give the keep's upper landing standing headroom; verify `keep_war_room_stairs.lua` (spec *The keep's stairs to the war room*)
- [x] 4.2 Walk the collision tree along the bake's rays with an early exit; verify `collide.bvh_matches_checking_every_box`, `render_golden`, and the audit's load time (spec *Same light, less waiting*)
- [x] 4.3 Re-run the audit after the changes; the composite goes from 51.5 to 80.5 with no pillar lower than before
