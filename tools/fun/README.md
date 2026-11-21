# Fun audit

`tools/fun/audit.py` applies the **game-fun-evaluator** skill (`.claude/skills/game-fun-evaluator/SKILL.md`)
to Banner & Bastion. Bots play Harrowmark in the headless engine and probes measure how the
commander feels. A windowed pass times the load and captures frames. The six pillars and the
composite fun score come out in the skill's report format.

```sh
cmake --build --preset dev && cmake --build --preset release   # the windowed pass uses release
python3 tools/fun/audit.py                                     # 20 episodes per persona, ~70 s on 4 cores
python3 tools/fun/audit.py --episodes 4 --label quick --no-window
python3 tools/fun/audit.py --label after --compare build/dev/fun/baseline/report.json
python3 -I tools/tests/test_fun_metrics.py                     # the metrics' own tests
```

Reports go to `build/<preset>/fun/<label>/`: `report.json`, `report.md` and the frames. Results
worth keeping go to [`docs/fun-audit/`](../../docs/fun-audit/README.md).

## Pieces

| Piece | What it is |
|---|---|
| `tests/fun/mods/fun_eval/` | A mod that is never shipped. The audit copies it into each run's user folder and enables it with `--mod fun_eval`. |
| `scripts/fun/telemetry.lua` | Hooks the game without changing it: sounds started, subtitles, entities used, new screen text and a state sample each tick. Output is `FUN {json}` log lines, stamped with simulation ticks. Nothing reads the wall clock. |
| `scripts/fun/bot.lua` | A bot drives the commander through the action layer, as a player would, within the limits of its persona (below). |
| `scripts/fun/nav.lua` | Harrowmark's route graph: places joined by ways, each way with the moves that cover it (walking, doors, the keep's stairs and lift, the wall stair, the Raven Tower's spiral). If a way is blocked, the bot turns back and plans again, and the audit reports that way. |
| `scripts/fun/goals.lua` | The 7 goals that score an episode: through both gates, into the Great Keep, speak with the castellan, the war room, the inner wall-walk, the top of the Raven Tower. |
| `scripts/fun/episode.lua`, `tests/fun/episode_<persona>.lua` | One 300 s episode per run. Each run uses a different `--seed`. |
| `scripts/fun/probes.lua`, `tests/fun/probe_feel.lua` | Fixed set-ups that measure response latency, motion curves, human-timed jumps, feedback channels and use reach. |
| `tests/fun/probe_frames.lua` | The windowed pass: clicks the menu's map button, then waits for control. It takes frame pairs with and without the overlay for the contrast measurement. |
| `tools/fun/metrics.py` | The pillar scores as pure functions over the telemetry. |

## Personas

| Persona | Skill role | How it plays |
|---|---|---|
| novice | B_rand | Random actions at 2–4 Hz (move, run, turn, jump, use, crouch) with no idea where anything is. |
| human | B_human | Sees its own position 200 ms late (±35 ms jitter). Turns at most 6 rad/s, aims with a 2° error and presses 200 ms after deciding to. Watches for the prompt and steps closer when there is none. Slows down to stop on a spot. Heads for the nearest goal it can see. Routes by its own noisy idea of how long each way takes, fixed for the episode. |
| oracle | B_opt | No delay and exact aim. Tries every order of the goals and takes the cheapest, by measured route times. Sprints whenever it can. It does not know about level bugs: a blocked way surfaces as a snag, as it would for a player. |
| spam | SpamBot | Holds forward, run and sprint straight at the nearest goal and mashes use. |

## How the pillars map onto this game

Banner & Bastion's first milestone is a walkable castle: you can't be hurt and you can't fail.
Where a measure needs threats or health, the audit reports it as n/a instead of inventing a
number.

| Pillar | Measured as | Notes |
|---|---|---|
| **Feel** | Response ticks of move, jump, crouch and use (× 33.3 ms). Drop rate of 40 jumps pressed by human timing (uniform ±100 ms) around a landing and around leaving a ledge. Start and stop ramps (2–10 ticks counts as eased). Apex hang. Feedback channels per event: audio, new screen text, things moving, the camera's own motion, particles. | The simulation runs at 30 Hz, so the floor is 1 tick (33.3 ms). Looking around is applied every rendered frame. |
| **Flow** | W = share of the 7 goals reached in 300 s: novice, human, oracle, plus the human's spread across runs. | M1 has no failure state, so the human proxy finishes everything: W = 100%, σ = 0. This is outside the skill's 40–75% sweet spot by design. Challenge arrives with skirmishes (roadmap M2+). |
| **Choice** | Entropy of the voluntary verbs (use, sprint, jump, crouch; walking and running are basic locomotion) used by the human and the oracle, averaged with the entropy of the routes the human takes to the war room (stairs or lift). Also the spam ratio W(spam)/W(human). | Jump and crouch have no use on the routes yet. |
| **Pace** | T(t) at 1 Hz = ½ stimulus density (sounds, lines, cards, uses; footsteps excluded) + ½ input rate (presses, verbs, turning). CV and the longest stretch above 0.9. | No threats or health, so those terms are 0. Near-misses are n/a. |
| **Fairness** | Use reach vs drawn size (angular sweep; people against their sprite's figure). Text contrast of prompts, subtitles and cards in rendered frames, found by difference with the overlay hidden. Designed ways that turn out to be blocked. Shake as a share of the 70° view. | Hurtbox ratio and telegraph time are n/a: nothing deals damage. |
| **Loop** | Time from the menu's map button to control (release build, windowed). Share of 5 s windows with some feedback (micro cadence). Gaps between goals the game acknowledges with a line, a card or a sound (meso cadence). | Death-to-retry is n/a: there is no failure. Under xvfb the windowed pass uses Mesa's software renderer, so load times are an upper bound. |

The composite is the skill's formula: `100 × feel^.25 × flow^.25 × fair^.20 × (0.40 + 0.25 choice + 0.20 pace + 0.15 loop)`.
Each sub-score is 1 inside the skill's target and falls linearly to 0 at a stated limit (see
`metrics.py`).

## Caveats

- Bots are proxies. They play the routes the graph knows, they don't get lost or curious, and
  they don't read the story. Use the audit to compare builds and to find feel, fairness and
  reachability problems. It doesn't replace playtesting with people.
- Determinism: an episode is reproducible from its seed and the build. Changing the game's
  movement changes the trajectories, and so the details of every episode.
