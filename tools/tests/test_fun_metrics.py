#!/usr/bin/env python3
"""Self-tests of tools/fun/metrics.py (the fun audit's pillar scores) on hand-made telemetry.

usage: python3 -I tools/tests/test_fun_metrics.py"""
import sys
sys.dont_write_bytecode = True  # the tools are imported from the repository: keep it clean
import importlib.util  # noqa: E402
import os  # noqa: E402
import unittest  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("metrics", os.path.join(HERE, "..", "fun", "metrics.py"))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def probe(**over):
    p = {
        "latency": {"move": 1, "jump": 1, "crouch": 1, "use": 1},
        "motion": {"start": [3.5] * 15, "stop": [0.0] * 15, "run_speed": 3.5,
                   "jump_vy": [3.9 - 0.327 * i for i in range(24)] + [0.0] * 6},
        "timing": {"trials": 40, "dropped": 0},
        "feedback": [{"event": "door", "impact": "core", "n": 3}, {"event": "slam", "impact": "high", "n": 4}],
        "reach": [], "camera_max": None,
    }
    p.update(over)
    return p


def episode(done, ticks=9000, extra=()):
    return list(extra) + [{"k": "end", "done": done, "goals": 7, "ticks": ticks, "t": ticks}]


class Shapes(unittest.TestCase):
    def test_band_and_limits(self):
        self.assertEqual(m.band(0.5, 0.4, 0.75, 0.4), 1.0)
        self.assertAlmostEqual(m.band(0.95, 0.4, 0.75, 0.4), 0.5)
        self.assertEqual(m.band(2.0, 0.4, 0.75, 0.4), 0.0)
        self.assertEqual(m.at_most(30, 33.4, 67), 1.0)
        self.assertAlmostEqual(m.at_most(50.2, 33.4, 67), 0.5)
        self.assertEqual(m.at_least(5, 3.5, 1.0), 1.0)
        self.assertAlmostEqual(m.at_least(2.25, 3.5, 1.0), 0.5)

    def test_entropy(self):
        h, pmax = m.entropy_norm([5, 5, 5, 5])
        self.assertAlmostEqual(h, 1.0)
        self.assertAlmostEqual(pmax, 0.25)
        self.assertEqual(m.entropy_norm([4, 0]), (0.0, 1.0))
        self.assertEqual(m.entropy_norm([0, 0, 0]), (0.0, 1.0))


class Feel(unittest.TestCase):
    def test_boxcar_motion_scores_zero(self):
        score, d = m.feel(probe())
        self.assertEqual(d["accel_ticks"], 1)
        self.assertEqual(d["decel_ticks"], 1)
        self.assertEqual(d["apex_hang"], 0.0)
        self.assertEqual(d["scores"]["motion"], 0.0)
        self.assertEqual(d["scores"]["latency"], 1.0)

    def test_eased_motion_and_hang(self):
        ramp = [0.7, 1.4, 2.1, 2.8, 3.3] + [3.5] * 10
        stop = [2.4, 1.4, 0.6, 0.2, 0.0] + [0.0] * 10
        vy = [3.9, 3.57, 3.24, 2.91, 2.58, 2.25, 1.92, 1.59, 1.26, 0.93, 0.75, 0.57, 0.39, 0.21, 0.03, -0.15,
              -0.33, -0.51, -0.69, -0.87, -1.2, -1.53, -1.86, -2.19, 0.0, 0.0]
        _, d = m.feel(probe(motion={"start": ramp, "stop": stop, "run_speed": 3.5, "jump_vy": vy}))
        self.assertEqual(d["accel_ticks"], 5)
        self.assertEqual(d["decel_ticks"], 4)
        self.assertEqual(d["apex_hang"], 1.0)
        self.assertEqual(d["scores"]["motion"], 1.0)

    def test_drops_and_channels(self):
        _, d = m.feel(probe(timing={"trials": 40, "dropped": 25}))
        self.assertAlmostEqual(d["drop_rate"], 0.625)
        self.assertEqual(d["scores"]["drop"], 0.0)
        self.assertEqual(d["channels_core"], 3)
        self.assertAlmostEqual(d["scores"]["sense"], 1.0)
        _, d = m.feel(probe(latency={"move": 2, "jump": None, "crouch": 1, "use": 1}))
        self.assertGreater(d["latency_ms"], 67)


class Flow(unittest.TestCase):
    def test_no_challenge(self):
        ws = {"novice": [0.0] * 4, "human": [1.0] * 4, "oracle": [1.0] * 4, "spam": [0.3] * 4}
        score, d = m.flow(ws)
        self.assertEqual(d["scores"]["gap"], 1.0)
        self.assertEqual(d["scores"]["sigma"], 0.0)
        self.assertAlmostEqual(d["scores"]["sweet"], 0.375)

    def test_sweet_spot(self):
        ws = {"novice": [0.0, 0.1], "human": [0.45, 0.75], "oracle": [1.0, 1.0], "spam": [0.2, 0.2]}
        score, d = m.flow(ws)
        self.assertAlmostEqual(score, 1.0)

    def test_w_of(self):
        self.assertAlmostEqual(m.w_of(episode(7)), 1.0)
        self.assertEqual(m.w_of([]), 0.0)


class Choice(unittest.TestCase):
    def test_routes_and_verbs(self):
        ep = [{"k": "edge", "a": "lift_hall", "b": "war_lift", "t": 1}, {"k": "goal", "id": "war_room", "t": 2},
              {"k": "press", "action": "use", "t": 3}, {"k": "act", "action": "sprint", "t": 4},
              {"k": "act", "action": "crouch", "t": 5}, {"k": "press", "action": "jump", "t": 6}]
        ep2 = [{"k": "edge", "a": "stair_foot", "b": "war_stair", "t": 1}, {"k": "goal", "id": "war_room", "t": 2}]
        self.assertEqual(m.war_room_routes([ep, ep2]), {"lift": 1, "stairs": 1})
        counts = m.verb_counts([ep])
        self.assertEqual(counts, {"use": 1, "sprint": 1, "jump": 1, "crouch": 1})
        score, d = m.choice({"human": [ep, ep2], "oracle": []}, {"human": [1.0], "spam": [0.2]})
        self.assertAlmostEqual(d["h_norm"], 1.0)
        self.assertEqual(score, 1.0)


class Pace(unittest.TestCase):
    def test_waves(self):
        flat = [{"k": "s", "t": t, "yaw": 0.0} for t in range(0, 300, 3)]
        quiet = episode(7, ticks=300, extra=flat)
        self.assertEqual(m.cv([0.1] * 5), 0.0)
        self.assertEqual(m.longest_above([1, 1, 0.2, 1], 0.9), 2)
        series = m.tension_series(quiet)
        self.assertEqual(len(series), 11)
        self.assertTrue(all(v == 0 for v in series))
        steps = [{"k": "audio", "sound": "sounds/footstep.wav", "t": t} for t in range(0, 300, 15)]
        self.assertEqual(m.stimulus_events(steps), [])
        self.assertEqual(len(m.stimulus_events(steps, steps=True)), 20)


class Fair(unittest.TestCase):
    def test_contrast_of_an_overlay(self):
        bg = [(200, 190, 180)] * 400
        on = [(82, 78, 74)] * 300 + [(250, 236, 200)] * 100  # panel at 41 % and bright text
        c = m.text_contrast(on, bg)
        self.assertGreater(c, 3.5)
        self.assertIsNone(m.text_contrast(bg, bg))
        self.assertAlmostEqual(m.contrast(1.0, 0.0), 21.0)

    def test_use_ratio_and_snags(self):
        r = {"dist": 2.0, "w": 2.2 * 2.0 / 1.92, "h": 3.2 * 2.0 / 1.92}
        self.assertAlmostEqual(m.use_ratio(r, (2.2, 3.2, 0.16)), 1.0, places=3)
        pr = probe(reach=[dict(r, name="keep.door")])
        blocked = [{"k": "blocked", "a": "x", "b": "y", "t": 1}, {"k": "stuck", "t": 1}]
        score, d = m.fair(pr, {"oracle": [blocked], "human": []}, {"v": 5.0}, None)
        self.assertEqual(d["blocked_ways"], ["x-y"])
        self.assertEqual(d["scores"]["snags"], 0.5)
        self.assertEqual(d["scores"]["contrast"], 1.0)


class Loop(unittest.TestCase):
    def test_cadence_and_ttfa(self):
        ep = [{"k": "goal", "id": "a", "t": 900}, {"k": "subtitle", "t": 905},
              {"k": "audio", "sound": "sounds/footstep.wav", "t": 100}] + episode(7, ticks=1800)
        self.assertEqual(m.acknowledged_goals(ep), [900])
        score, d = m.loop(1.0, {"human": [ep]})
        self.assertEqual(d["scores"]["ttfa"], 1.0)
        self.assertEqual(d["meso_gaps_s"], [30.0, 30.0])
        self.assertEqual(d["scores"]["meso"], 1.0)
        self.assertEqual(m.loop(None, {"human": []})[1]["scores"]["ttfa"], 0.0)

    def test_composite(self):
        ones = {k: 1.0 for k in ("feel", "flow", "fair", "choice", "pace", "loop")}
        self.assertAlmostEqual(m.composite(ones), 100.0)
        self.assertEqual(m.composite(dict(ones, feel=0.0)), 0.0)
        self.assertIn("Engaging", m.verdict(80))
        self.assertIn("Broken", m.verdict(10))
        self.assertIn("Functional", m.verdict(60))
        self.assertIn("High-craft", m.verdict(90))


if __name__ == "__main__":
    unittest.main()
