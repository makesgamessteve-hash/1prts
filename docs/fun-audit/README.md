# Fun audits

Results of `tools/fun/audit.py` (the game-fun-evaluator skill; see [`tools/fun/README.md`](../../tools/fun/README.md)
for what is measured and how).

## 2026-11-21: first audit, and the fixes it led to

Both runs use the same harness: 20 episodes per persona, the release build for load time, and
frames under xvfb. The baseline is the game before these changes, built from a checkout of the
previous commit.

| Pillar | Baseline | After | What changed |
|---|---|---|---|
| Feel | 0.32 | **0.97** | A jump pressed up to 120 ms before landing is kept (input buffer). Up to 100 ms after walking off an edge still jumps (coyote time). Human-timed drops fell from 62% to 0%. Walking eases in over 4 ticks and stops over 4 (it was a boxcar). Gravity eases at the top of a jump. Footsteps in time with a head bob, a jump rustle, a landing thud with a view dip, and a shake when heavy things land nearby. Guards turn to whoever talks to them and murmur. The lift settles with a thud. The portcullis slams. Core events now answer on 2.4 channels on average (was 1.1); heavy ones on 2.7 (was 0.3). |
| Flow | 0.68 | 0.68 | Unchanged, as it should be. M1 has no failure state: the human proxy reaches every goal, every run. Challenge comes with the skirmish milestones; it was not faked here with a timer. |
| Choice | 0.78 | **1.00** | The keep's stairs to the war room could not be climbed standing up: the floor above the upper landing left 1.7 m of headroom. With the slab trimmed, the stairs are a real alternative to the lift (the human proxy now splits 10/10 between them). Jump and crouch still have no use on Harrowmark's routes. |
| Pace | 0.50 | 0.50 | Unchanged. Without threats the tension index has long calm stretches and short peaks (CV 1.2; the skill wants 0.25–0.85). |
| Fairness | 0.77 | **0.98** | No more blocked ways. Small props (levers, call buttons, the winches) are picked a little beyond their edges (use-box ratio 1.15–1.30, was 0.88–1.00). The camera's shake stays under 2.5% of the view. |
| Loop | 0.67 | **1.00** | Time from the map button to control fell from 4.1 s to 0.9 s. The sun bake now walks the collision tree along each ray and stops at the first hit; the result is the same. Title cards for each part of the castle, and the castellan's four orders tracked, announced and ticked off on the board, give a meso goal every 5–50 s (gaps were up to 140 s). Some feedback now happens in 99% of 5 s windows (was 27%). |
| **Composite** | **51.5** | **80.5** | From "functional, lacks spark" to "engaging, solid game loop". |

Reports: [baseline](2026-11-21-baseline.md) ([json](2026-11-21-baseline.json)),
[after](2026-11-21-after.md) ([json](2026-11-21-after.json)).

### What is left

- **Flow and pace need stakes.** Raids that test whether you barred the gates, a Wildborn
  warband on the ridge road, a reason to hurry. These are the next milestones' job.
- **Jump and crouch need a purpose:** low passages, gaps, obstacles worth vaulting.
- **The keep door's top half is beyond use reach** from where you usually stand (use-box ratio
  0.92). That's realistic, but it could be looked at.
- **Bots don't replace people.** They follow routes they know. Load times under software GL are
  an upper bound.
