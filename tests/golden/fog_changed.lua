-- Spec: Visual regression. The first golden view with a different fog color: comparing this
-- shot with the approach_retro reference must fail (tools/golden/run.py --expect-mismatch).
-- MIT License.
return {
  name = "golden view with a changed fog color",
  max_seconds = 10,
  run = function(t)
    prts.render.settings{ preset = "retro", dither = true, snap = true, affine = false, fog = true }
    prts.scene.env{ fog_color = { 0.25, 0.55, 0.30 } }
    -- far down the road, so the castle stands in the dusk haze (fog starts at 90 m)
    t.place(0, prts.scene.height(0, 250), 250, 0, 0.06)
    t.ticks(2)
    t.screenshot("approach_retro")
  end,
}
