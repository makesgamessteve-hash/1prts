---
name: game-fun-evaluator
description: Instrument, playtest, quantitatively evaluate, and iteratively improve computer game "fun" during remote coding and testing sessions using telemetry, bot personas, and mathematical game-feel/flow metrics.
---

# Game Fun Evaluation & Improvement Skill

Use this skill whenever tasked with building, playtesting, auditing, or improving a game in a remote coding environment (browser/Playwright, headless engine loop, Pygame, Godot, Unity CLI, or Node/Canvas). Because an AI coding agent cannot subjectively "feel" boredom or excitement, this skill translates established game design theory (Game Feel, Csikszentmihalyi's Flow, Sid Meier's Meaningful Choices, and Koster's Mastery Curves) into **measurable telemetry, automated bot benchmarks, and deterministic code remediations**.

---

## 1. Execution Workflow

Always execute this five-step closed loop during a game evaluation session:

1. **Instrument**: Inject a lightweight, non-invasive telemetry hook into the game loop and input handler.
2. **Simulate**: Run automated playtest sessions across three distinct synthetic bot personas (plus headless fast-forward ticks if supported) and capture visual frames/DOM/canvas states.
3. **Measure**: Calculate the six quantitative pillar scores defined in Section 4.
4. **Diagnose**: Match any failing metric against the Remediation Playbook in Section 5 to identify root-cause design or feel flaws.
5. **Patch & Re-verify**: Implement targeted code improvements, re-run the test harness, and confirm metric progression without regressing existing pillars.

---

## 2. Step 1: Telemetry & State Instrumentation

Before judging a game, ensure the runtime exposes structured state snapshots and event logs. If no telemetry interface exists, inject a minimal global state bridge (e.g., `window.__GAME_EVAL__` for web games or a JSONL logger on `stdout`/file for native games).

### Required Telemetry Schema

Track the following events and frame summaries at ${}60\text{ Hz}$ (or once per simulation tick):

* **Lifecycle Timestamps**: `boot_ms`, `first_interactive_ms`, `run_start_ms`, `game_over_ms`, `restart_ready_ms`.
* **Input Events**: `timestamp_ms`, `action_id`, `accepted` (boolean indicating whether the action triggered a state change or was dropped during cooldown/recovery), `visual_response_frame` (first frame where velocity, sprite, or particle count changes).
* **Entity & Collision Snapshots**:
  * Player position $(x, y)$, velocity $(v_x, v_y)$, acceleration $(a_x, a_y)$, health/shield state.
  * Sprite bounding box $(w_s, h_s)$ vs. physics hurtbox/hitbox $(w_h, h_h)$.
  * Active hazards/enemies count, nearest threat distance $d_{\min}$ (in player-widths).
* **Juice & Feedback Counters**: Active particle count, camera offset/shake magnitude, hitstop/freeze-frame ticks, audio triggers fired per event, easing curve type (linear vs. non-linear).
* **Economy/Decision Events**: Choices offered vs. choices selected (`weapon_id`, `upgrade_id`, `move_type`, `path_id`), score/reward deltas $\Delta R$.

---

## 3. Step 2: Automated Playtest Personas

A single test script cannot measure skill expression or fairness. Run at least $N \ge 20$ episodes (or ${}60\text{ seconds}$ of gameplay per persona) using three synthetic agents:

1. **Persona A — Novice / Noise Bot ($B_{\text{rand}}$)**:
   * Emits random valid inputs at ${}2\text{--}4\text{ Hz}$ with no spatial awareness.
   * *Purpose*: Establishes the skill floor. Detects games that play themselves or lack meaningful failure states.
2. **Persona B — Human-Proxy Heuristic Bot ($B_{\text{human}}$)**:
   * Uses simple local rules (move toward rewards, evade threats within radius $r$, attack nearest target) **constrained by simulated human limitations**: a reaction delay of $\tau_r = 200\text{ ms}$ (${}12$ frames at ${}60\text{ FPS}$) and timing jitter of $\pm 35\text{ ms}$.
   * *Purpose*: Approximates an engaged average player. Measures flow, pacing, near-misses, and input forgiveness.
3. **Persona C — Oracle / Frame-Perfect Bot ($B_{\text{opt}}$)**:
   * Zero artificial latency ($\tau_r = 0\text{ ms}$), reads exact entity velocities/hitboxes, selects greedy-optimal or lookahead actions.
   * *Purpose*: Establishes the skill ceiling. Detects unavoidable damage, RNG lockouts, and dominant degenerate strategies.

---

## 4. Step 3: Quantitative Fun Metrics

Compute each of the six pillar metrics on a normalized $[0, 1]$ scale, then compute the composite **Fun Score ($S_{\text{fun}}$)**.

### Pillar 1: Kinesthetics & Game Feel ($M_{\text{feel}}$)

Measures micro-level responsiveness, forgiveness, and multisensory "juice."

1. **Input-to-Visual Latency ($L_{\text{vis}}$)**: Time from input event to measurable state/pixel delta.
   * Target: $L_{\text{vis}} \le 33.4\text{ ms}$ ($\le 2$ frames at ${}60\text{ FPS}$). Fail if $L_{\text{vis}} > 67\text{ ms}$.
2. **Input Drop Rate ($R_{\text{drop}}$)**: Fraction of inputs by $B_{\text{human}}$ ignored within ${}100\text{ ms}$ prior to state readiness (lack of input buffering) or ${}100\text{ ms}$ after leaving a ledge/state (lack of coyote time).
   * Target: $R_{\text{drop}} < 0.05$.
3. **Motion Non-Linearity**: Check player velocity profiles. Instantaneous boxcar velocity without acceleration/deceleration curves or instant linear interpolation feels robotic unless intentionally retro-grid based. Verify presence of easing, apex hang-time (gravity reduction near jump peak when $|v_y| < \epsilon$), or snappy damping.
4. **Multisensory Feedback Ratio ($F_{\text{sense}}$)**: Average number of distinct feedback channels triggered per significant event (hit, pickup, jump, death) across $\{ \text{animation/squash-stretch}, \text{particles}, \text{camera impulse/hitstop}, \text{audio}, \text{UI pop} \}$.
   * Target: $F_{\text{sense}} \ge 2.5$ channels for core actions; $\ge 3.5$ for high-impact events.

### Pillar 2: Flow & Skill Discrimination ($M_{\text{flow}}$)

Measures whether player skill meaningfully governs outcomes without triviality or unfairness. Let $W(\cdot)$ be survival time or normalized objective completion in $[0, 1]$:

$$\Delta_{\text{skill}} = \overline{W}(B_{\text{opt}}) - \overline{W}(B_{\text{rand}})$$

* **Skill Gap Target**: $\Delta_{\text{skill}} \ge 0.55$.
  * If $\overline{W}(B_{\text{rand}}) > 0.35$, the game lacks challenge or plays itself.
  * If $\overline{W}(B_{\text{opt}}) < 0.70$, the game contains unavoidable damage, broken physics, or excessive RNG.
* **Human Flow Sweet Spot**: $\overline{W}(B_{\text{human}})$ should fall within $[0.40, 0.75]$ on default difficulty, with run-to-run standard deviation $\sigma_W \in [0.10, 0.25]$ (high enough for variety, low enough to feel fair).

### Pillar 3: Meaningful Choices & Strategic Entropy ($M_{\text{choice}}$)

Measures whether available actions, tools, or paths are genuinely viable rather than dominated by a single optimal spam action. Across $K$ distinct voluntary actions/upgrades ($K \ge 2$), record empirical usage probabilities $p_1, p_2, \dots, p_K$ under $B_{\text{human}}$ and $B_{\text{opt}}$, and compute normalized Shannon entropy:

$$H_{\text{norm}} = -\frac{1}{\ln K} \sum_{i=1}^{K} p_i \ln(p_i)$$

* **Target**: $H_{\text{norm}} \ge 0.70$, and $\max_i (p_i) \le 0.65$ (excluding basic locomotion).
* **Spam Penalty**: Test a fourth degenerate sub-run where the bot spams only the single highest-DPS/primary button. If `SpamBot` achieves $\ge 0.80 \times \overline{W}(B_{\text{human}})$, the game suffers from a dominant strategy collapse.

### Pillar 4: Pacing, Tension Waves & Near-Misses ($M_{\text{pace}}$)

Fun requires oscillation between high-intensity tension and brief cognitive recovery—never a flat line. Compute a frame-by-frame **Tension Index** $T(t)$ sampled at ${}1\text{ Hz}$:

$$T(t) = w_1 \cdot \text{ThreatDensity}(t) + w_2 \cdot \text{InputRate}(t) + w_3 \cdot \left(1 - \frac{\text{Health}(t)}{\text{MaxHealth}}\right)$$

* **Wave Variance**: $T(t)$ must exhibit peaks and troughs. Coefficient of variation $CV_T = \frac{\sigma_T}{\mu_T}$ should satisfy ${}0.25 \le CV_T \le 0.85$.
  * $CV_T < 0.20$ indicates monotonous, flat pacing.
  * Monotonically maxing out $T(t) > 0.90$ for $> 20\text{ s}$ causes player exhaustion.
* **Near-Miss Frequency ($N_{\text{miss}}$)**: Count instances per minute where a hazard passes within ${}1.0\times$ to ${}1.5\times$ the player's hurtbox radius without colliding.
  * Target: ${}2 \le N_{\text{miss}} \le 12\text{ min}^{-1}$ during active combat/navigation. Near-misses generate excitement and perceived mastery.

### Pillar 5: Fairness & Perceptual Clarity ($M_{\text{fair}}$)

Players should never blame the game for a failure. Audit via code inspection and rendered frame capture:

1. **Hitbox Generosity**:
   * Player hurtbox-to-visual-sprite area ratio $\alpha_{\text{hurt}} = \frac{A_{\text{hurtbox}}}{A_{\text{sprite}}}$ must satisfy ${}0.50 \le \alpha_{\text{hurt}} \le 0.85$. (Never $\alpha_{\text{hurt}} > 1.0$).
   * Player attack/pickup hitbox ratio $\alpha_{\text{hit}}$ must satisfy ${}1.05 \le \alpha_{\text{hit}} \le 1.35$.
2. **Telegraph Lead Time ($t_{\text{warn}}$)**: Fast or high-damage enemy attacks/spawns must display a visual or audio telegraph for $t_{\text{warn}} \ge 300\text{ ms}$ ($\ge 1.5 \times \tau_r$) before active damage frames begin.
3. **Visual Contrast**: Foreground interactive entities (player, projectiles, pickups) must maintain WCAG luminance contrast $\ge 3.5:1$ against background tiles/effects. Screen shake amplitude must never exceed ${}4\%$ of viewport height or obscure projectile trajectories.

### Pillar 6: Loop Friction & Time-to-Fun ($M_{\text{loop}}$)

Measures respect for player time and "one more try" compulsion.

* **Time-to-First-Action ($t_{\text{TTFA}}$)**: From page/binary load (or clicking Play) to active player control: target $t_{\text{TTFA}} \le 3.0\text{ s}$.
* **Death-to-Retry Latency ($t_{\text{retry}}$)**: From failure state to active control in a fresh run via single keypress/click: target $t_{\text{retry}} \le 1.0\text{ s}$.
* **Reward Cadence**: Micro-feedback every ${}1\text{--}5\text{ s}$ (hits, coins, combo ticks), meso-goals every ${}30\text{--}90\text{ s}$ (wave clear, level-up, upgrade choice, checkpoint).

### Composite Fun Score Formula

Combine the six pillars using a weighted geometric-arithmetic blend so that a catastrophic ${}0$ in responsiveness or fairness tanks the overall score:

$$S_{\text{fun}} = 100 \times \left( M_{\text{feel}}^{0.25} \cdot M_{\text{flow}}^{0.25} \cdot M_{\text{fair}}^{0.20} \right) \times \left( 0.40 + 0.25 M_{\text{choice}} + 0.20 M_{\text{pace}} + 0.15 M_{\text{loop}} \right)$$

*(Normalize against maximum value ${}1.0$ so $S_{\text{fun}} \in [0, 100]$).*

* $S_{\text{fun}} < 50$: Broken prototype / unenjoyable. Fix foundational feel, fairness, or broken difficulty before adding content.
* ${}50 \le S_{\text{fun}} < 75$: Functional game, lacks spark or balance. Apply juice, pacing waves, and strategic tuning.
* ${}75 \le S_{\text{fun}} < 88$: Engaging, solid game loop. Polish edge cases and depth synergies.
* $S_{\text{fun}} \ge 88$: High-craft, compelling gameplay.

---

## 5. Step 4: Diagnostic & Auto-Remediation Playbook

When a metric misses its target during remote testing, apply the corresponding engineering fix directly in code:

| Symptom / Failing Metric | Root Cause | Concrete Code Remediation |
| :--- | :--- | :--- |
| $R_{\text{drop}} \ge 0.05$ or sluggish controls | Unbuffered discrete inputs; strict ground checks | Implement an **Input Buffer** queue (${}120\text{ ms}$ TTL) for jumps/attacks/dashes and **Coyote Time** (${}100\text{ ms}$ grace window after leaving grounded state). |
| $F_{\text{sense}} < 2.5$ ("flat/dead" impact) | State changes only decrement numbers | Add **Hitstop** (${}30\text{--}65\text{ ms}$ simulation pause on heavy impacts), directional **Particle Bursts** (${}8\text{--}20$ particles with drag), decaying **Trauma Screen Shake** ($\text{offset} = \text{max} \cdot \text{trauma}^2$), sprite **Squash & Stretch** (${}1.25\times$ / ${}0.8\times$ tweening back over ${}120\text{ ms}$), and pitch-randomized procedural/sampled SFX ($\pm 8\%$ pitch variance). |
| $\overline{W}(B_{\text{rand}}) > 0.35$ | Game is too passive or forgiving | Introduce active spatial threats that force movement, tighten time/resource pressure, or prevent stationary camping. |
| $\overline{W}(B_{\text{opt}}) < 0.70$ or $\overline{W}(B_{\text{human}}) < 0.30$ | Unavoidable overlapping hazards, instant spawns, or bullet-hell lockouts | Enforce minimum spawn distance $d_{\text{spawn}} \ge 25\%$ of arena width from player; add ${}400\text{ ms}$ spawn warning indicators; cap simultaneous projectile velocity/density; add brief post-hit invulnerability frames (${}500\text{--}1000\text{ ms}$). |
| $H_{\text{norm}} < 0.70$ or `SpamBot` succeeds | Dominant single action; no situational trade-offs | Add cooldowns, ammo/stamina tension, enemy archetypes resistant to primary spam, or synergy multipliers rewarding alternating actions (e.g., action A primes target for action B). |
| $CV_T < 0.20$ (monotonous pacing) | Constant linear spawn timer (e.g., `spawnEvery(1000ms)`) | Replace flat timers with a **Wave Director**: alternate ${}20\text{--}35\text{ s}$ escalating intensity waves with ${}5\text{--}8\text{ s}$ breather/reward collection troughs, punctuated by mini-climaxes. |
| $\alpha_{\text{hurt}} > 0.85$ ("unfair hits") | Physics collider matches full sprite `width * height` | Shrink player damage collider by ${}20\text{--}30\%$ inward from sprite edges; expand pickup/melee hitboxes by ${}15\%$. |
| $t_{\text{retry}} > 1.0\text{ s}$ | Heavy scene reloads or unskippable death modals | Reset game state dictionary in-memory without reloading assets; bind `Space`/`Enter`/`R` to instant restart after a ${}250\text{ ms}$ accidental-input guard. |

---

## 6. Step 5: Remote Session Execution Checklist

When running in a remote CLI or headless browser environment (e.g., Playwright + Chromium, Puppeteer, Pygame `SDL_VIDEODRIVER=dummy`, Godot `--headless`):

1. **Deterministic Seed Control**: Expose a PRNG seed parameter (`?seed=42` or `--seed 42`) so before-and-after code changes are evaluated on identical enemy/level sequences alongside randomized generalization seeds.
2. **Accelerated Simulation**: If physics uses a fixed timestep (`dt = 1/60`), decouple `step(dt)` from `requestAnimationFrame` in test mode so ${}60\text{ minutes}$ of bot gameplay can execute in $< 10\text{ seconds}$ of wall-clock time. Run at least one real-time (${}60\text{ FPS}$) pass to verify actual frame render times ($< 16.6\text{ ms}$) and visual contrast via screenshots.
3. **Visual Frame Audit**: Capture screenshots at `boot`, `peak_tension`, and `game_over`. Inspect (via vision capabilities or pixel luminance histograms) for UI overlap, off-screen clipping, unreadable text, or invisible projectiles.

---

## 7. Required Evaluation Output Format

At the end of every playtest cycle, output a concise diagnostic table before and after applying code fixes:

```text
=== GAME FUN AUDIT REPORT ===
Runs Executed: [N] per persona (Novice, Human-Proxy @ 200ms, Optimal)

Pillar Scores (0.00 - 1.00):
- Kinesthetics & Feel (M_feel):   [0.XX] | Latency: [X]ms, DropRate: [X]%, Channels: [X.X]
- Flow & Skill Gap (M_flow):      [0.XX] | Win/Surv: Rand=[X]%, Human=[X]%, Opt=[X]%
- Strategic Entropy (M_choice):   [0.XX] | H_norm=[0.XX], SpamBot Ratio=[0.XX]
- Pacing & Tension (M_pace):      [0.XX] | Tension CV=[0.XX], Near-Misses=[X.X]/min
- Fairness & Clarity (M_fair):    [0.XX] | Hurtbox Ratio=[0.XX], Telegraph=[X]ms
- Loop Friction (M_loop):         [0.XX] | TTFA=[X.X]s, Retry=[X.X]s

COMPOSITE FUN SCORE: [XX.X] / 100

Top Bottlenecks Identified:
1. [Metric -> Specific mechanical cause]
2. [Metric -> Specific mechanical cause]

Planned Code Interventions:
1. [Exact file/system change & target metric delta]
```