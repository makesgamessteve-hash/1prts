-- Spec (commander-locomotion): Wall at sprint speed. The commander sprints straight at the
-- keep's east wall for 3 seconds and stays on the near side of it. MIT License.
return {
  name = "sprinting into the keep's wall does not pass through it",
  run = function(t)
    local c = t.commander()
    t.place(18, 0, -4, math.pi / 2, 0) -- east of the keep, facing west (-x)
    t.ticks(2)
    t.input.hold("sprint", 3)
    t.input.hold("move_forward", 3)
    t.ticks(30)
    local x = t.pos(c)
    t.expect(x < 17.0, "the commander ran towards the wall").is_true()
    t.ticks(65)
    x = t.pos(c)
    t.expect(x >= 12.0, "still outside the wall's outer face (x >= 12)").is_true()
    t.expect(x < 12.6, "pressed against the wall").is_true()
    t.expect(t.in_cell(c, "outside"), "outside the keep").is_true()
  end,
}
