-- Spec: Visual regression. The first golden view with a different fog color: comparing this
-- shot with the approach_retro reference must fail (tools/golden/run.py --expect-mismatch).
-- MIT License.
return {
  name = "golden view with a changed fog color",
  max_seconds = 10,
  run = function(t)
    prts.render.settings{ preset = "retro", dither = true, snap = true, affine = false, fog = true }
    prts.scene.env{ fog_color = { 0.25, 0.55, 0.30 } }
    t.place("start", 0.05)
    t.ticks(2)
    t.screenshot("approach_retro")
  end,
}
