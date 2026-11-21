-- Determinism (simulation-core "Record and replay", "Two identical runs"): a Proving Ground walk
-- through the keep's door, across the great hall and up the hoist lift to the war room.
-- After the start position is set (before the first tick, so it is part of the replay's start
-- snapshot) the walk is driven only by actions and look direction, which is exactly what a
-- replay records. CTest records it with --record, replays it with --replay (every tick's state
-- hash must match) and records it a second time (both recordings must be identical).
return {
  name = "a recorded walk through the keep door and up the hoist lift",
  max_seconds = 90,
  run = function(t)
    local c = t.commander()
    t.place("keep_door")
    t.face("keep.door", 1.1, 1.6, 0)
    t.ticks(2)
    t.expect(t.target(), "looking at").equals("keep.door")
    t.input.press("use")
    t.ticks(30)

    -- walks in the view direction until cond holds
    local function walk(yaw, cond)
      t.look(yaw, 0)
      t.input.hold("move_forward", 20)
      t.wait_until(cond, 20)
      t.input.release("move_forward")
      t.ticks(2)
    end
    walk(0, function() local _, _, z = t.pos(c) return z < 6.95 end)         -- north into the hall
    walk(math.pi / 2, function() local x = t.pos(c) return x < -6.4 end)      -- west, between the pillars and the open door
    walk(0, function() local _, _, z = t.pos(c) return z < 0.0 end)          -- north, to the lift gate
    t.expect(t.in_zone(c, "keep.great_hall"), "in the great hall").is_true()

    t.look(math.pi / 2, 0)
    t.input.hold("move_forward", 2.4)                                       -- onto the platform
    t.wait_until(function() return t.parent(c) == "keep.lift.platform" end, 4)
    t.ticks(40)
    t.face("keep.lift.lever")
    t.ticks(1)
    t.expect(t.target(), "looking at").equals("keep.lift.lever")
    t.input.press("use")
    t.ticks(2)
    t.wait_until(function() local _, y = t.pos(c) return y > 9.1 end, 10)
    t.ticks(60)                                                             -- the gate slides up
    t.look(-math.pi / 2, 0)
    t.input.hold("move_forward", 3)
    t.wait_until(function() return t.parent(c) == "world" end, 4)
    t.ticks(30)
    t.expect(t.in_zone(c, "keep.floor3.war_room"), "in the war room").is_true()
  end,
}
