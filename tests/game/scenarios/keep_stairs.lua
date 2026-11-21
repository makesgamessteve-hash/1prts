-- Spec (commander-locomotion): Stairs. Two flights and a landing up to the second floor.
return {
  name = "commander climbs the switchback stairs to the second floor",
  run = function(t)
    local c = t.commander()
    t.place("stair_base")
    t.input.hold("move_forward", 4.2)
    t.ticks(126)
    local _, y_landing = t.pos(c)
    t.expect(y_landing, "landing height").near(2.45, 0.05)
    t.look(math.pi, 0)                 -- turn round: now facing +z
    t.input.hold("move_right", 1.6)    -- side-step to the second flight
    t.ticks(48)
    t.input.hold("move_forward", 4)
    t.ticks(120)
    local _, y = t.pos(c)
    t.expect(y, "second floor height").near(4.7, 0.05)
    t.expect(t.in_zone(c, "keep.floor2"), "on the second floor").is_true()
  end,
}
