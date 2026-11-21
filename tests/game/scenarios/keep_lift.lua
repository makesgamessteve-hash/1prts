-- Spec (commander-locomotion): Riding the keep's hoist lift to the war room.
-- Also covers simulation-core "Riding a moving frame" and xr "Pulling the lift lever"
-- (the same use path an XR controller ray triggers).
-- ctest-labels: smoke
return {
  name = "commander rides the keep's hoist lift to the war room",
  final_cell = "keep.floor3",
  max_seconds = 60,
  run = function(t)
    local c = t.commander()
    t.place("keep_hall_lift")
    t.ticks(2)
    t.input.hold("move_right", 1.2)    -- step in front of the gate (facing west)
    t.ticks(36)
    t.look(math.pi / 2, 0)
    t.input.hold("move_forward", 2.4)  -- onto the platform
    t.wait_until(function() return t.parent(c) == "keep.lift.platform" end, 4)
    t.ticks(40)
    t.face("keep.lift.lever")
    t.ticks(1)
    t.expect(t.target(), "looking at").equals("keep.lift.lever")
    t.input.press("use")
    t.ticks(2)
    -- the platform rises 9 m at 1.6 m/s while the commander stands on it
    t.wait_until(function() local _, y = t.pos(c) return y > 9.1 end, 10)
    t.expect(t.parent(c), "riding").equals("keep.lift.platform")
    t.ticks(60)                        -- the gate slides up
    t.look(-math.pi / 2, 0)            -- face east, out of the shaft
    t.input.hold("move_forward", 3)
    t.wait_until(function() return t.parent(c) == "world" end, 4)
    t.ticks(30)
    t.expect(t.in_zone(c, "keep.floor3.war_room"), "in the war room").is_true()
    local _, y = t.pos(c)
    t.expect(y, "war room floor").near(9.2, 0.05)
  end,
}
