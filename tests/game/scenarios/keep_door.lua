-- Spec (commander-locomotion): Opening a door; the commander walks into the great hall.
return {
  name = "commander opens the keep's door and enters the great hall",
  run = function(t)
    local c = t.commander()
    t.place("keep_door")
    t.face("keep.door", 1.1, 1.6, 0)
    t.ticks(2)
    t.expect(t.target(), "looking at").equals("keep.door")
    t.input.press("use")
    t.ticks(30)
    t.look(0, 0)
    t.input.hold("run", 3)
    t.input.hold("move_forward", 3)
    t.wait_until(function() return t.in_zone(c, "keep.great_hall") end, 4)
    t.expect(t.in_zone(c, "keep.great_hall"), "in the great hall").is_true()
  end,
}
