#!/usr/bin/env python3
"""Fun audit of Banner & Bastion (the game-fun-evaluator skill, .claude/skills/).

Runs the bot personas of tests/fun against Harrowmark in the headless engine (N episodes
each, one seed per episode), the feel probes, and a windowed pass for load time and frames,
then scores the six pillars (tools/fun/metrics.py) and prints the audit report.

  python3 tools/fun/audit.py                      # build/dev, 20 episodes per persona
  python3 tools/fun/audit.py --episodes 4 --label quick
  python3 tools/fun/audit.py --compare build/dev/fun/baseline/report.json

The report (report.json, report.md) and the frames go to build/<preset>/fun/<label>/.
MIT License.
"""
import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import metrics  # noqa: E402

PERSONAS = ("novice", "human", "oracle", "spam")
MOD = os.path.join(ROOT, "tests", "fun", "mods", "fun_eval")


def exe_of(build):
    for name in ("1stprts", "1stprts.exe"):
        p = os.path.join(build, "bin", name)
        if os.path.exists(p):
            return p
    raise SystemExit("no 1stprts executable in %s/bin (build it first)" % build)


def user_dir(out, name):
    d = os.path.join(out, "runs", name)
    shutil.rmtree(d, ignore_errors=True)
    shutil.copytree(MOD, os.path.join(d, "mods", "fun_eval"))
    return d


def parse(text):
    events, passed = [], False
    for line in text.splitlines():
        i = line.find("FUN {")
        if i >= 0:
            events.append(json.loads(line[i + 4:]))
        elif "scenario PASSED" in line:
            passed = True
    return events, passed


def run(exe, scenario, out, name, seed, extra=(), timed=False, prefix=()):
    cmd = list(prefix) + [exe, "--scenario", os.path.join(ROOT, "tests", "fun", scenario), "--user",
                          user_dir(out, name), "--mod", "fun_eval", "--seed", str(seed)] + list(extra)
    if not prefix:
        cmd.insert(1, "--headless")
    t0 = time.monotonic()
    marks = {}
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, cwd=ROOT)
    lines = []
    for line in p.stdout:
        lines.append(line)
        if timed:
            for k in ("menu", "click", "control"):
                if '"k":"%s"' % k in line:
                    marks[k] = time.monotonic() - t0
    p.wait()
    events, passed = parse("".join(lines))
    if not passed:
        sys.stderr.write("".join(lines[-30:]))
        raise RuntimeError("%s (seed %d) failed" % (scenario, seed))
    return events, marks


def probe_results(events):
    first = lambda k: next((e for e in events if e.get("k") == k), None)  # noqa: E731
    # shake as a share of the view's height: the pitch kick over the 70 degree field of view
    cams = [abs(e["camp"]) / metrics.FOV_Y for e in events if e.get("k") == "s" and e.get("camp") is not None]
    return {
        "latency": first("probe_latency"),
        "motion": first("probe_motion"),
        "timing": first("probe_timing_summary"),
        "feedback": [e for e in events if e.get("k") == "probe_feedback"],
        "reach": [e for e in events if e.get("k") == "probe_reach"],
        "camera_max": max(cams) if cams else None,
    }


def load_rgb(path):
    from PIL import Image  # only the frame audit needs Pillow
    img = Image.open(path).convert("RGB")
    return list(img.get_flattened_data()) if hasattr(img, "get_flattened_data") else list(img.getdata())


def frame_contrasts(frames):
    out = {}
    try:
        import PIL  # noqa: F401
    except ImportError:
        return out
    for f in sorted(os.listdir(frames)):
        if f.endswith("_on.png"):
            name = f[:-7]
            off = os.path.join(frames, name + "_off.png")
            if os.path.exists(off):
                out[name] = metrics.text_contrast(load_rgb(os.path.join(frames, f)), load_rgb(off))
    return out


def sprite_alpha():
    """The guard sprite's opaque figure in its front idle frame, in metres (sprites/crown_guard)."""
    try:
        from PIL import Image
    except ImportError:
        return None
    img = Image.open(os.path.join(ROOT, "data", "base", "sprites", "crown_guard.png")).convert("RGBA")
    frame = img.crop((0, 0, 32, 64))
    box = frame.getchannel("A").getbbox()
    if not box:
        return None
    return {"w": (box[2] - box[0]) / 32.0 * (1.85 * 32 / 64), "h": (box[3] - box[1]) / 64.0 * 1.85}


def fmt(v, spec="%.2f"):
    return "n/a" if v is None else spec % v


def report_text(r):
    p, d = r["pillars"], r["details"]
    f, fl, c, pa, fa, lo = d["feel"], d["flow"], d["choice"], d["pace"], d["fair"], d["loop"]
    hit = [a for a in fa["alpha_hit"].values() if a is not None]
    lines = [
        "=== GAME FUN AUDIT REPORT === (%s)" % r["label"],
        "Runs Executed: %d per persona (Novice, Human-Proxy @ 200ms, Optimal; plus SpamBot)" % r["episodes"],
        "",
        "Pillar Scores (0.00 - 1.00):",
        "- Kinesthetics & Feel (M_feel):   %.2f | Latency: %.0fms, DropRate: %.0f%%, Channels: %.1f (high-impact %.1f)"
        % (p["feel"], f["latency_ms"], 100 * f["drop_rate"], f["channels_core"], f["channels_high"]),
        "- Flow & Skill Gap (M_flow):      %.2f | Win/Surv: Rand=%.0f%%, Human=%.0f%%, Opt=%.0f%% (sigma %.2f)"
        % (p["flow"], 100 * fl["w_rand"], 100 * fl["w_human"], 100 * fl["w_opt"], fl["sigma_human"]),
        "- Strategic Entropy (M_choice):   %.2f | H_norm=%.2f, SpamBot Ratio=%.2f" % (p["choice"], c["h_norm"], c["spam_ratio"]),
        "- Pacing & Tension (M_pace):      %.2f | Tension CV=%.2f, Near-Misses=n/a (no hazards in M1)"
        % (p["pace"], pa["tension_cv"]),
        "- Fairness & Clarity (M_fair):    %.2f | Hurtbox Ratio=n/a (no damage in M1), Use-box Ratio=%s, Contrast=%s:1, "
        "Telegraph=n/a, Blocked ways=%d" % (p["fair"], fmt(min(hit) if hit else None), fmt(fa["contrast_min"], "%.1f"),
                                           len(fa["blocked_ways"])),
        "- Loop Friction (M_loop):         %.2f | TTFA=%ss, Retry=n/a (no failure state in M1)"
        % (p["loop"], fmt(lo["ttfa_s"], "%.1f")),
        "",
        "COMPOSITE FUN SCORE: %.1f / 100  (%s)" % (r["score"], metrics.verdict(r["score"])),
        "",
        "Details:",
        "  motion: start %d ticks, stop %d ticks, apex hang %s" % (f["accel_ticks"], f["decel_ticks"],
                                                                   "yes" if f["apex_hang"] else "no"),
        "  feedback channels by event: %s" % ", ".join("%s %d" % kv for kv in f["feedback"].items()),
        "  verbs used (human+oracle): %s; war room reached by %s" % (
            ", ".join("%s %d" % kv for kv in c["verbs"].items()), ", ".join("%s %d" % kv for kv in c["routes"].items())),
        "  use-box ratios: %s" % ", ".join("%s %s" % (k, fmt(v)) for k, v in fa["alpha_hit"].items()),
        "  overlay contrast by view: %s" % ", ".join("%s %s" % (k, fmt(v, "%.1f")) for k, v in fa["contrast"].items()),
        "  blocked ways: %s (stuck events %d)" % (", ".join(fa["blocked_ways"]) or "none", fa["stuck_events"]),
        "  meso gaps (s): %s; micro coverage %.2f" % (lo["meso_gaps_s"][:12], lo["micro_coverage"]),
        "  sub-scores: %s" % json.dumps({k: {a: round(b, 2) for a, b in v["scores"].items()} for k, v in d.items()}),
    ]
    return "\n".join(lines)


def compare_text(old, new):
    lines = ["", "=== BEFORE -> AFTER (%s -> %s) ===" % (old["label"], new["label"])]
    for k in ("feel", "flow", "choice", "pace", "fair", "loop"):
        a, b = old["pillars"][k], new["pillars"][k]
        lines.append("- M_%-7s %.2f -> %.2f  (%+.2f)%s" % (k, a, b, b - a, "  REGRESSION" if b < a - 1e-9 else ""))
    lines.append("- Composite  %.1f -> %.1f  (%+.1f)" % (old["score"], new["score"], new["score"] - old["score"]))
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--build", default=os.path.join(ROOT, "build", "dev"))
    ap.add_argument("--ttfa-build", default=None, help="build for the windowed pass (default: build/release if built)")
    ap.add_argument("--episodes", type=int, default=20)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--label", default="audit")
    ap.add_argument("--out", default=None)
    ap.add_argument("--compare", default=None, help="an earlier report.json to compare with")
    ap.add_argument("--no-window", action="store_true", help="skip the windowed pass (no display)")
    args = ap.parse_args()

    exe = exe_of(args.build)
    out = args.out or os.path.join(args.build, "fun", args.label)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    t0 = time.monotonic()

    jobs = [(p, s) for p in PERSONAS for s in range(1, args.episodes + 1)]
    episodes = {p: [] for p in PERSONAS}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futs = {pool.submit(run, exe, "episode_%s.lua" % p, out, "%s_%d" % (p, s), s): (p, s) for p, s in jobs}
        futs[pool.submit(run, exe, "probe_feel.lua", out, "probes", 1)] = ("probes", 1)
        probe = None
        for fut in concurrent.futures.as_completed(futs):
            who, seed = futs[fut]
            events, _ = fut.result()
            if who == "probes":
                probe = probe_results(events)
            else:
                episodes[who].append((seed, events))
    for p in PERSONAS:
        episodes[p] = [ev for _, ev in sorted(episodes[p], key=lambda x: x[0])]

    ttfa, contrasts = None, {}
    frames = os.path.join(out, "frames")
    if not args.no_window and shutil.which("xvfb-run"):
        wbuild = args.ttfa_build or (os.path.join(ROOT, "build", "release")
                                     if os.path.isdir(os.path.join(ROOT, "build", "release", "bin")) else args.build)
        _, marks = run(exe_of(wbuild), "probe_frames.lua", out, "frames", 1,
                       extra=("--hidden", "--front-end", "--out", frames), timed=True, prefix=("xvfb-run", "-a"))
        if "click" in marks and "control" in marks:
            ttfa = marks["control"] - marks["click"]
        contrasts = frame_contrasts(frames)

    ws = {p: [metrics.w_of(ep) for ep in episodes[p]] for p in PERSONAS}
    pillars, details = {}, {}
    pillars["feel"], details["feel"] = metrics.feel(probe)
    pillars["flow"], details["flow"] = metrics.flow(ws)
    pillars["choice"], details["choice"] = metrics.choice(episodes, ws)
    pillars["pace"], details["pace"] = metrics.pace(episodes)
    pillars["fair"], details["fair"] = metrics.fair(probe, episodes, contrasts, sprite_alpha())
    pillars["loop"], details["loop"] = metrics.loop(ttfa, episodes)
    result = {"label": args.label, "episodes": args.episodes, "pillars": pillars, "details": details,
              "score": metrics.composite(pillars), "w": ws, "seconds": round(time.monotonic() - t0, 1)}
    text = report_text(result)
    if args.compare and os.path.exists(args.compare):
        with open(args.compare) as fh:
            text += "\n" + compare_text(json.load(fh), result)
    print(text)
    print("\n(%d episodes in %.0f s; report in %s)" % (len(jobs), result["seconds"], out))
    with open(os.path.join(out, "report.json"), "w") as fh:
        json.dump(result, fh, indent=1, sort_keys=True)
    with open(os.path.join(out, "report.md"), "w") as fh:
        fh.write("```text\n" + text + "\n```\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
