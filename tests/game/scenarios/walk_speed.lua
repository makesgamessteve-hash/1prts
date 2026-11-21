-- Spec (commander-locomotion): Walking speed. 10 s of walking on flat ground covers 14 m.
return {
  name = "commander walks 14 m in 10 s",
  run = function(t)
    local c = t.commander()
    t.place("start")
    t.ticks(5)
    local _, _, z0 = t.pos(c)
    t.input.hold("move_forward", 10)
    t.ticks(300)
    local _, _, z1 = t.pos(c)
    t.expect(z0 - z1, "distance").near(14.0, 0.2)
  end,
}
