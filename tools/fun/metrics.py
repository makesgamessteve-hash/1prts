"""Fun audit metrics: the six pillars of the game-fun-evaluator skill computed from the
telemetry of tests/fun (see tools/fun/README.md for how each one maps onto the game).

Every function here is pure: it takes parsed telemetry events (dicts, as the engine logs them)
and returns numbers, so tools/tests/test_fun_metrics.py can check them without the engine.
Scores are piecewise linear: 1.0 inside the skill's target, falling to 0.0 at a stated limit.
MIT License.
"""
import math
import statistics

TICK_MS = 1000.0 / 30.0
FOV_Y = math.radians(70.0)  # the default vertical field of view (gfx)
GOALS = 7


def clamp01(v):
    return max(0.0, min(1.0, v))


def band(value, lo, hi, width):
    """1 inside [lo, hi]; falls linearly to 0 at `width` outside it."""
    if lo <= value <= hi:
        return 1.0
    d = lo - value if value < lo else value - hi
    return clamp01(1.0 - d / width)


def at_most(value, good, bad):
    """1 at or below `good`, 0 at or above `bad`."""
    if value <= good:
        return 1.0
    return clamp01(1.0 - (value - good) / (bad - good))


def at_least(value, good, bad):
    """1 at or above `good`, 0 at or below `bad`."""
    if value >= good:
        return 1.0
    return clamp01((value - bad) / (good - bad))


def mean(xs):
    xs = [x for x in xs if x is not None]
    return sum(xs) / len(xs) if xs else 0.0


def of(events, kind):
    return [e for e in events if e.get("k") == kind]


# --- Pillar 1: kinesthetics and feel -------------------------------------------------------

def ramp_ticks(speeds, target, frac):
    """Ticks until speed reaches frac * target (starting) or falls below it (stopping)."""
    for i, v in enumerate(speeds):
        if (frac >= 0.5 and v >= frac * target) or (frac < 0.5 and v <= frac * target):
            return i + 1
    return len(speeds) + 1


def eased(ticks):
    """A start or stop is eased (not a boxcar) if it takes 2..10 ticks (67..333 ms)."""
    if ticks <= 1:
        return 0.0
    return at_most(ticks, 10, 20)


def apex_hang(vy):
    """1 if gravity is weaker near the top of the jump than on the way up (hang time)."""
    flight = []
    for v in vy:  # the jump until it lands (the vertical speed is 0 again on the ground)
        if flight and flight[-1] < 0 and v >= 0:
            break
        flight.append(v)
    vy = flight
    acc = [(vy[i + 1] - vy[i]) * 30.0 for i in range(len(vy) - 1)]
    rising = [abs((vy[i + 1] - vy[i]) * 30.0) for i in range(len(vy) - 1) if vy[i + 1] > 1.5]
    apex = [abs((vy[i + 1] - vy[i]) * 30.0) for i in range(len(vy) - 1) if abs(vy[i]) < 1.0 and abs(vy[i + 1]) < 1.0]
    if not rising or not apex or not acc:
        return 0.0
    return 1.0 if mean(apex) <= 0.8 * mean(rising) else 0.0


def feel(probe):
    lat = probe["latency"]
    ticks = [lat.get(k) for k in ("move", "jump", "crouch", "use")]
    worst = [t if t is not None else 30 for t in ticks]
    latency_ms = mean(worst) * TICK_MS
    timing = probe["timing"]
    drop = timing["dropped"] / timing["trials"] if timing["trials"] else 1.0
    m = probe["motion"]
    accel = ramp_ticks(m["start"], m["run_speed"], 0.9)
    decel = ramp_ticks(m["stop"], m["run_speed"], 0.1)
    hang = apex_hang(m["jump_vy"])
    motion = mean([eased(accel), eased(decel), hang])
    fb = probe["feedback"]
    core = [f["n"] for f in fb if f["impact"] == "core"]
    high = [f["n"] for f in fb if f["impact"] == "high"]
    f_core, f_high = mean(core), mean(high)
    s = {
        "latency": at_most(latency_ms, 33.4, 67.0),
        "drop": at_most(drop, 0.05, 0.5),
        "motion": motion,
        "sense": 0.5 * clamp01(f_core / 2.5) + 0.5 * clamp01(f_high / 3.5),
    }
    return mean(s.values()), {
        "latency_ms": latency_ms, "drop_rate": drop, "accel_ticks": accel, "decel_ticks": decel, "apex_hang": hang,
        "channels_core": f_core, "channels_high": f_high, "scores": s,
        "feedback": {f["event"]: f["n"] for f in fb},
    }


# --- Pillar 2: flow and skill discrimination ----------------------------------------------

def w_of(episode):
    end = of(episode, "end")
    if not end:
        return 0.0
    return end[-1]["done"] / end[-1]["goals"]


def flow(ws):
    wr, wh, wo = mean(ws["novice"]), mean(ws["human"]), mean(ws["oracle"])
    sigma = statistics.pstdev(ws["human"]) if len(ws["human"]) > 1 else 0.0
    gap = wo - wr
    s = {
        "gap": clamp01(gap / 0.55),
        "rand": at_most(wr, 0.35, 0.70),
        "opt": at_least(wo, 0.70, 0.0),
        "sweet": band(wh, 0.40, 0.75, 0.40),
        "sigma": band(sigma, 0.10, 0.25, 0.10),
    }
    return mean(s.values()), {"w_rand": wr, "w_human": wh, "w_opt": wo, "sigma_human": sigma, "gap": gap, "scores": s}


# --- Pillar 3: meaningful choices -----------------------------------------------------------

VERBS = ("use", "sprint", "jump", "crouch")  # run is basic locomotion, like walking


def entropy_norm(counts):
    total = sum(counts)
    k = len(counts)
    if total == 0 or k < 2:
        return 0.0, 1.0
    ps = [c / total for c in counts]
    h = -sum(p * math.log(p) for p in ps if p > 0) / math.log(k)
    return h, max(ps)


def verb_counts(episodes):
    counts = {v: 0 for v in VERBS}
    for ep in episodes:
        for e in ep:
            if e.get("k") == "press" and e.get("action") in counts:
                counts[e["action"]] += 1
            elif e.get("k") == "act" and e.get("action") in counts:
                counts[e["action"]] += 1
    return counts


def war_room_routes(episodes):
    """How each episode got up to the war room: the way done just before that goal."""
    routes = {"lift": 0, "stairs": 0}
    for ep in episodes:
        last = None
        for e in ep:
            if e.get("k") == "edge":
                last = (e["a"], e["b"])
            if e.get("k") == "goal" and e.get("id") == "war_room":
                if last == ("lift_hall", "war_lift"):
                    routes["lift"] += 1
                elif last == ("stair_foot", "war_stair"):
                    routes["stairs"] += 1
                break
    return routes


def choice(episodes, ws):
    played = episodes["human"] + episodes["oracle"]
    verbs = verb_counts(played)
    h_verbs, p_max = entropy_norm(list(verbs.values()))
    routes = war_room_routes(episodes["human"])
    h_routes, _ = entropy_norm(list(routes.values()))
    h = mean([h_verbs, h_routes])
    wh = mean(ws["human"])
    spam = mean(ws["spam"]) / wh if wh > 0 else 0.0
    s = {"entropy": clamp01(h / 0.70), "dominant": at_most(p_max, 0.65, 1.0), "spam": at_most(spam, 0.80, 1.0)}
    return mean(s.values()), {"h_norm": h, "h_verbs": h_verbs, "h_routes": h_routes, "p_max": p_max, "verbs": verbs,
                              "routes": routes, "spam_ratio": spam, "scores": s}


# --- Pillar 4: pacing ------------------------------------------------------------------------

AMBIENT = ("loop", "music")


def stimulus_events(ep, steps=False):
    """Times of things the game shows or plays in answer to the player. Footsteps count as
    micro feedback (steps=True) but not as tension."""
    out = []
    quiet = AMBIENT if steps else AMBIENT + ("footstep",)
    for e in ep:
        k = e.get("k")
        if k == "audio" and not any(a in e.get("sound", "") for a in quiet):
            out.append(e["t"])
        elif k in ("subtitle", "goal", "use"):
            out.append(e["t"])
        elif k == "ui":
            out.append(e["t"])
    return out


def tension_series(ep):
    """T(t) at 1 Hz: half stimulus density (sounds, lines, goals, things used), half input
    rate (presses, verbs started, turning). No threats or health exist in this milestone, so
    their terms are 0 and the weights are shared by the two that do."""
    end = of(ep, "end")
    if not end:
        return []
    seconds = max(1, end[-1]["ticks"] // 30)
    stim = [0] * (seconds + 1)
    inputs = [0.0] * (seconds + 1)
    for t in stimulus_events(ep):
        stim[min(seconds, t // 30)] += 1
    for e in ep:
        if e.get("k") in ("press", "act"):
            inputs[min(seconds, e["t"] // 30)] += 1
    samples = of(ep, "s")
    for a, b in zip(samples, samples[1:]):
        d = abs((b["yaw"] - a["yaw"] + math.pi) % (2 * math.pi) - math.pi)
        inputs[min(seconds, b["t"] // 30)] += d / math.pi
    return [0.5 * clamp01(s / 3.0) + 0.5 * clamp01(i / 2.0) for s, i in zip(stim, inputs)]


def cv(xs):
    if len(xs) < 2:
        return 0.0
    m = mean(xs)
    return statistics.pstdev(xs) / m if m > 0 else 0.0


def longest_above(xs, level):
    best = run = 0
    for x in xs:
        run = run + 1 if x > level else 0
        best = max(best, run)
    return best


def pace(episodes):
    series = [tension_series(ep) for ep in episodes["human"]]
    series = [s for s in series if s]
    cvs = [cv(s) for s in series]
    longest = max((longest_above(s, 0.9) for s in series), default=0)
    c = mean(cvs)
    s = {"waves": band(c, 0.25, 0.85, 0.25), "exhaustion": at_most(longest, 20, 60)}
    return mean(s.values()), {"tension_cv": c, "longest_peak_s": longest, "near_miss": None, "scores": s}


# --- Pillar 5: fairness and clarity ----------------------------------------------------------

def srgb_lin(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def luminance(rgb):
    r, g, b = rgb[:3]
    return 0.2126 * srgb_lin(r) + 0.7152 * srgb_lin(g) + 0.0722 * srgb_lin(b)


def contrast(l1, l2):
    hi, lo = max(l1, l2), min(l1, l2)
    return (hi + 0.05) / (lo + 0.05)


def text_contrast(on, off):
    """WCAG contrast of overlay text against its own backing panel, from two frames of the same
    view: with the overlay (`on`) and without (`off`), both lists of (r, g, b) pixels. A panel
    darkens what is behind it by a constant factor (found as the most common on/off ratio);
    text pixels are much brighter than the panel would make them. The worst case is used: the
    dimmest tenth of the text against the brightest twentieth of the panel."""
    pairs = [(a, b) for a, b in zip(on, off) if max(abs(a[i] - b[i]) for i in range(3)) > 6]
    if len(pairs) < 20:
        return None
    ratio = lambda a, b: (sum(a[:3]) + 3.0) / (sum(b[:3]) + 3.0)  # noqa: E731
    counts = {}
    for a, b in pairs:
        r = ratio(a, b)
        if sum(b[:3]) > 30 and r < 0.95:  # darkened: behind the panel
            counts[round(r, 2)] = counts.get(round(r, 2), 0) + 1
    if not counts:
        return None
    factor = max(counts, key=lambda r: (counts[r], -r))
    panel = sorted(luminance(a) for a, b in pairs if abs(ratio(a, b) - factor) < 0.06)
    text = sorted(luminance(a) for a, b in pairs if sum(a[:3]) > sum(b[:3]) * factor + 90)
    if not panel or not text:
        return None
    return contrast(text[len(text) // 10], panel[int(len(panel) * 0.95)])


# What the player sees of each probed usable thing: width, height and depth of the drawn box (m),
# from props/door.lua, props/lift.lua and props/portcullis.lua. People are sprites (their
# figure is measured from the sprite sheet).
VISUAL = {"keep.door": (2.2, 3.2, 0.16), "harrowmark.kings_gate.winch": (0.3, 0.6, 0.3), "keep.lift.call1": (0.16, 0.5, 0.16)}


def use_ratio(r, visual):
    """Area of the use ray's reach around a thing over the area the player sees, both as seen
    from the probe's spot (a drawn box's front face is nearer than the point looked at)."""
    w, h, depth = visual
    d = r["dist"]
    near = d / max(0.05, d - depth / 2.0)
    return (r["w"] * r["h"]) / (w * h * near * near)


def fair(probe, episodes, contrasts, sprite_alpha):
    reach = {r["name"]: r for r in probe["reach"]}
    alphas = {}
    for name, r in reach.items():
        if name.startswith("npc."):
            alphas[name] = (r["w"] * r["h"]) / (sprite_alpha["w"] * sprite_alpha["h"]) if sprite_alpha else None
        elif name in VISUAL:
            alphas[name] = use_ratio(r, VISUAL[name])
    hit = [band(a, 1.05, 1.35, 0.35) for a in alphas.values() if a is not None]
    blocked = set()
    stuck = 0
    for ep in episodes["oracle"] + episodes["human"]:
        for e in ep:
            if e.get("k") == "blocked":
                blocked.add((e["a"], e["b"]))
            if e.get("k") == "stuck":
                stuck += 1
    cs = [c for c in contrasts.values() if c is not None]
    worst = min(cs) if cs else None
    s = {
        "hit": mean(hit) if hit else 0.0,
        "contrast": at_least(worst, 3.5, 1.0) if worst is not None else 0.0,
        "snags": 1.0 / (1.0 + len(blocked)),
    }
    cam = probe.get("camera_max")
    if cam is not None:
        s["shake"] = at_most(cam, 0.04, 0.08)
    return mean(s.values()), {"alpha_hit": alphas, "contrast_min": worst, "contrast": contrasts,
                              "blocked_ways": sorted("%s-%s" % b for b in blocked), "stuck_events": stuck,
                              "alpha_hurt": None, "telegraph_ms": None, "shake": cam, "scores": s}


# --- Pillar 6: loop friction -----------------------------------------------------------------

def acknowledged_goals(ep):
    """Goal times the game itself answered within a second (a line, a sound, a message)."""
    out = []
    events = [e for e in ep if e.get("k") in ("subtitle", "audio", "ui", "goal")]
    for g in of(ep, "goal"):
        for e in events:
            if g["t"] <= e["t"] <= g["t"] + 30 and e is not g:
                if e["k"] == "subtitle" or (e["k"] == "ui" and any(not x.startswith("[") for x in e.get("texts", []))) \
                   or (e["k"] == "audio" and not any(a in e.get("sound", "") for a in AMBIENT + ("footstep",))):
                    out.append(g["t"])
                    break
    return out


def cadence(episodes):
    micro, meso = [], []
    for ep in episodes["human"]:
        end = of(ep, "end")
        if not end:
            continue
        ticks = end[-1]["ticks"]
        times = sorted(stimulus_events(ep, steps=True))
        windows = max(1, ticks // 150)
        hit = {t // 150 for t in times}
        micro.append(len([w for w in range(windows) if w in hit]) / windows)
        marks = [0] + acknowledged_goals(ep) + [ticks]
        gaps = [(b - a) / 30.0 for a, b in zip(marks, marks[1:])]
        meso.extend(gaps)
    return mean(micro), meso


def loop(ttfa_s, episodes):
    micro, gaps = cadence(episodes)
    meso = mean([at_most(g, 90, 180) for g in gaps]) if gaps else 0.0
    s = {"ttfa": at_most(ttfa_s, 3.0, 10.0) if ttfa_s is not None else 0.0, "micro": micro, "meso": meso}
    return mean(s.values()), {"ttfa_s": ttfa_s, "retry_s": None, "micro_coverage": micro,
                              "meso_gaps_s": [round(g, 1) for g in gaps], "scores": s}


# --- Composite ---------------------------------------------------------------------------------

def composite(p):
    core = (p["feel"] ** 0.25) * (p["flow"] ** 0.25) * (p["fair"] ** 0.20)
    return 100.0 * core * (0.40 + 0.25 * p["choice"] + 0.20 * p["pace"] + 0.15 * p["loop"])


def verdict(score):
    if score < 50:
        return "Broken prototype / unenjoyable: fix foundational feel, fairness, or broken difficulty first"
    if score < 75:
        return "Functional game, lacks spark or balance"
    if score < 88:
        return "Engaging, solid game loop"
    return "High-craft, compelling gameplay"
